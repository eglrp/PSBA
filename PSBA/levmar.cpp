
#include "stdafx.h"

#include <stdlib.h>
#include <time.h>

#include "psba.h"
#include "levmar.h"
#include "cl_psba.h"
#include "misc.h"
#include "cl_spdinv.h"
#include "cl_linearalg.h"

#include "sba_func.cpp"

extern dtype initErr;		//��ʼ���
extern clock_t cur_tm;
extern clock_t total_tm;	//�㷨��ʱ��
extern int itno;			//��ǰ��������
extern int jac_cnt;			//��¼jac�������
extern int Sinv_cnt;			//��¼Binv�������

extern clock_t alloc_tm;
extern clock_t genidx_tm;	//����������ʱ��
extern clock_t jac_tm;		//��¼jac������ʱ��
extern clock_t S_tm;		//����S_tm����ʱ��
extern clock_t Sinv_tm;		//��¼Binv������ʱ��
extern clock_t ex_tm;		//����ex��ʱ��
extern clock_t g_tm;
extern clock_t pred_ex_tm;

int *iidx;				//��СΪn2Dprojs��ָʾ��Ӧ2D���Ӧ��3D���������
int *jidx;				//��СΪn2Dprojs��ָʾ��Ӧ2D���Ӧ������������


//void update_UV_old(int cnp, int pnp, int n3Dpts, int nCams, dtype *U, dtype *V, dtype mu);
//dtype init_damping(int cnp, int pnp, int n3Dpts, int nCams, dtype *U, dtype *V, dtype tau);
void update_para(int n, dtype *old_p, dtype *dp, dtype *new_p);
dtype compute_rho(dtype ex_L2, dtype new_ex_L2, dtype mu, int nPara, dtype *dp, dtype *g);


/*
* Levenberg-Marquardt �㷨
*/
int levmar(
	//����,�����޸�
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά�ȣ�3D������ά�ȣ�2Dͼ�������ά��
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	dtype *finalErr)
{
	dtype ret;
	int iter_flag;			//�����ı�־
	int gooditer_cnt;
	dtype tau;
	dtype mu;				//damping����
	int nu;					//damping���ӵ���������
	dtype rho;

	dtype *UVdiag;			
	dtype *ex;				//ͶӰ���,(eij_x,eij_y)Ϊ3D��i�����j�ļ���ֵ�����ֵ֮������
	dtype *g;				//g=(J^t)*ex��	m*cnp+n*pnp��
	dtype *eab;				//���ڽ�S*dp=eab,  ������ea=ga-(YW^t)*gb�����dpa��Ȼ����eb=gb-Wt*dpa,���dpb
	dtype *dp;				//delta paramters���������+3D�����������
	dtype *new_p;			//�µĲ���

	dtype p_L2;				//������L2����
	dtype dp_L2;			//����������L2����
	dtype ex_L2;			//ͶӰ����L2����
	dtype new_ex_L2;		//


	tau = PSBA_INIT_MU;
	gooditer_cnt = 0;
	int nTotalParas =cnp*nCams + n3Dpts*pnp;
	int AblkSize = mnp*cnp;
	int BblkSize = mnp*pnp;
	int WblkSize = cnp*pnp;
	int nCamParas = cnp*nCams;
	int nPts3DParas = n3Dpts*pnp;
	bool first = true;

	//##########
	ex = (dtype*)emalloc(mnp*n2Dprojs * sizeof(dtype));
	UVdiag = (dtype*)emalloc(nTotalParas * sizeof(dtype));
	g = (dtype*)emalloc(nTotalParas * sizeof(dtype));
	eab = (dtype*)emalloc(nTotalParas * sizeof(dtype));
	dp = (dtype*)emalloc(nTotalParas * sizeof(dtype));
	new_p = (dtype*)emalloc(nTotalParas * sizeof(dtype));

	//����U,V,ͶӰ���,�Լ�ͶӰ����L2����
	cur_tm = clock();
	compute_exQT(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,psbaPtr->cams_buffer, psbaPtr->pts3D_buffer,ex);
	ex_L2 = compute_L2_sq(n2Dprojs*mnp, ex);
	initErr = ex_L2;
	//printf("init error: %le\n", ex_L2);

	//��ʼ����
	iter_flag = ITER_CONTINUE;
	for (; itno < 50 && iter_flag == ITER_CONTINUE; itno++)
	{
		//����Jacobi����
		compute_jacobiQT(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,NULL, NULL);
		compute_U(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,1,NULL);			//tested
		compute_V(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,1,NULL);			//tested
		compute_Wblks(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,iidx,jidx, 1, NULL);		//tested

		compute_g(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,1, g);
		//printBuf2D_blk(stdout, psbaPtr->queue, psbaPtr->JB_buffer, 0, 2,3, "JB00");
		//printBuf1D(stdout, psbaPtr->queue, psbaPtr->g_buffer, 0, 20, "ga");
		//printBuf1D(stdout, psbaPtr->queue, psbaPtr->g_buffer, nCamParas, 20, "gb");

		/* ��ʼ��damping���� */
		if (first) {
			mu = tau*maxElmOfUV(psbaPtr, nTotalParas,UVdiag);
			first = false;
			p_L2 = 1e+3;
			nu = 2;
			//printf("initial mu=%le\n", mu);
		}
		/*
		�������ѭ����ȷ�����µ�damping���ӣ�����������ֹ�����˳�
		*/
		while (1) {
			//printf("try\n");
			update_UV(psbaPtr, cnp, pnp, n3Dpts, nCams, mu, NULL, NULL);
			compute_Vinv(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, NULL);		//tested
			compute_Yblks(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,NULL,NULL, NULL);		//tested

			compute_S(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, NULL);
			compute_ea(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,NULL);		//compute ea=ga-(YW^t)*gb

			cur_tm = clock();
			ret = SPDinv(psbaPtr->queue, psbaPtr->kern_cholesky, psbaPtr->kern_trigMat_inv, psbaPtr->kern_trigMat_mul,
				psbaPtr->S_buffer, psbaPtr->diagBlkAux_buffer, psbaPtr->ret_buffer, nCamParas, NULL);

			if (ret == 0.0) {	//������ȷ
				/* ���dpa=(S^-1)*ga  */
				matVec_mul(psbaPtr->queue, psbaPtr->kern_matVec_mul,
					psbaPtr->S_buffer, psbaPtr->eab_buffer, psbaPtr->dp_buffer, nCams*cnp, nCams*cnp, NULL);

				//printBuf1D(stdout, psbaPtr->queue, psbaPtr->dp_buffer, 0, 20, "dpa");
				//printBuf2D_blk(stdout, psbaPtr->queue, psbaPtr->W_buffer, 0, 6, 3, "W00");


				//printBuf2D_blk(stdout, psbaPtr->queue, psbaPtr->U_buffer, 0, 6, 6, "U00");
				//printBuf2D_blk(stdout, psbaPtr->queue, psbaPtr->V_buffer, 0, 3, 3, "V00");


				/* ����eb, eb=gb-(W^t)*dpa  */
				compute_eb(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, NULL);		//&eab[nCamParas]
				//printBuf1D(stdout, psbaPtr->queue, psbaPtr->eab_buffer, nCamParas, 20, "eb");

				/* ���dpb=(V^-1)*eb, ��ΪVinv�ǿ���󣬲���ʹ��compute_matvecmul  */
				compute_dpb(psbaPtr, cnp, pnp, nCams, n3Dpts, dp);
				/* �������������L2���� */
				dp_L2 = compute_L2_sq(nTotalParas, dp);

				/*
				for (int i = nCamParas; i < nCamParas+20; i++)
					printf("%E\t", dp[i]);
				printf("\n");
				*/


				//printBuf1D(stdout, psbaPtr->queue, psbaPtr->dp_buffer,nCamParas,100, "dp");

				//*******�����ж�
				if (dp_L2 < p_L2*PSBA_STOP_THRESH*PSBA_STOP_THRESH) {
					//printf("dp is small enough,LM algorithm about to stop.\n");
					iter_flag = ITER_DP_NO_CHANGE;
					break;
				}
				if (dp_L2 >= (p_L2 + PSBA_STOP_THRESH) / PSBA_EPSILON_SQ) {
					printf("the matrix of the augmented normal equations is almost singular\n"
						"minimization should be restarted from the current solution with an increased damping term.\n");
					iter_flag = ITER_ERR;
					break;
				}

				//�ָ�UV�ĶԽ���
				restore_UVdiag(psbaPtr, cnp, pnp, n3Dpts, nCams);

				//��������dp��ȷ����ʹ����������cams_data,pts3D_data.
				compute_newp(psbaPtr, nCams*cnp, n3Dpts*pnp, NULL);

				//�����µ�ͶӰ���Լ�����L2����
				compute_exQT(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,
					psbaPtr->newCams_buffer, psbaPtr->newPts3D_buffer, ex);

				//printBuf1D(stdout, psbaPtr->queue, psbaPtr->ex_buffer, 0, 20, "ex");

				new_ex_L2 = compute_L2_sq(n2Dprojs*mnp, ex);

				rho = compute_rho(ex_L2, new_ex_L2, mu, nTotalParas, dp, g);

				printf("itno=%d\t\tErr=%.15E\t\trho=%f\t\tmu=%f\n", itno, new_ex_L2, rho,mu);

				//��rho>0ʱ���µ�damping����ȷ�������²���������ѭ���˳�
				if (rho > 0)
				{
					dtype tmp = 2 * rho - 1;
					tmp = 1.0 - tmp*tmp*tmp;
					mu = mu*((tmp >= (1.0 / 3.0)) ? tmp : (1.0 / 3.0));
					nu = 2;
					if (verbose) {
						fprintf(debug_file, "[iter %d] rho=%le, tmp=%le, err=%le, mu=%le\n", itno, rho, tmp, new_ex_L2, mu);
						fflush(debug_file);
					}
					//���²���
					update_p(psbaPtr, nCams*cnp, n3Dpts*pnp, new_p);
					p_L2 = compute_L2_sq(nTotalParas, new_p);
					ex_L2 = new_ex_L2;
					//����Ƿ��㹻�ã�ת��TR�㷨
					if (fabs(rho - 1) < (1.0 / 5.0))
					{
						gooditer_cnt++;
						if (gooditer_cnt >= 5) { iter_flag = ITER_TURN_TO_TR; break; }
					}
					else
						gooditer_cnt = 0;
					break;	//�˳��ڲ�whileѭ��
				}
				//else
				//	gooditer_cnt = 0;
			}
			else {
				gooditer_cnt = 0.0;
				//�ָ�UV�ĶԽ���
				restore_UVdiag(psbaPtr, cnp, pnp, n3Dpts, nCams);
				if (verbose)
					fprintf(debug_file, "#####   solve equation failed.\n"); fflush(debug_file);
			}
			//���е����˵��damping���ӻ�����ȷ,
			if(verbose)
				fprintf(debug_file, "[iter %d]  error not reduce.increase in dp is not accepted.\n",itno);
			mu *= nu;
			dtype nu2 = 2 * nu;
			if (nu2 <= nu) {
				printf("too many failed attempts to increase the damping factor.\n");
				iter_flag = ITER_ERR;
				break;
			}
			nu = nu2;
		}//�ڲ�whileѭ������
		/*�ж�����Ƿ��㹻С*/
		if (ex_L2 <= PSBA_STOP_THRESH)
			iter_flag = ITER_ERR_SMALL_ENOUGH;
	}	//����ѭ������

	*finalErr = ex_L2;
	free(g); free(UVdiag); free(eab); 
	free(dp); free(new_p); free(ex);

	 return iter_flag;
}


/*
���²���, new_p=old_p+dp
*/
inline void update_para(int n, dtype *old_p,dtype *dp, dtype *new_p)
{
	for (int i = 0; i < n; i++)
		new_p[i] = old_p[i]+ dp[i];
}

/*
����rho��ֵ
*/
inline dtype compute_rho(dtype ex_L2,dtype new_ex_L2,dtype mu, int nPara, dtype *dp,dtype *g)
{
	dtype sum = 0;
	for (int i = 0;i < nPara; i++)
	{
		sum += dp[i] * (mu*dp[i]+g[i]);
	}

	return (ex_L2-new_ex_L2)/sum;
}


