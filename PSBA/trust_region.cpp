
#include "stdafx.h"

#include <stdlib.h>
#include <time.h>

#include "psba.h"
#include "sba_func.h"
#include "cl_psba.h"
#include "misc.h"
#include "cl_spdinv.h"
#include "cl_cholmod.h"


#include "sba_func.cpp"


#define MAX_DELTA	10000

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
extern clock_t Sinv_tm ;		//��¼Binv������ʱ��
extern clock_t ex_tm;		//����ex��ʱ��
extern clock_t g_tm;
extern clock_t pred_ex_tm;


bool compute_PB(PSBA_structPtr psbaPtr, int cnp, int pnp, int mnp,
	int nCams, int n3Dpts, int n2Dprojs,
	int *iidx, int *jidx,int *blk_idx, dtype *lambda,
	dtype *P_B, dtype *UVdiag);
dtype compute_p_2(int nTotalParas, dtype pUtBpU, dtype pUtBpB, dtype pBtBpB, dtype delta,
	dtype *P_U, dtype *P_B, dtype *p, dtype *g);

dtype compute_DG_path(dtype *P_U, dtype *P_B, int size, dtype dk);

/*
* SBA��Trust-region �㷨
*/
int trust_region(
	//����,�����޸�
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά�ȣ�3D������ά�ȣ�2Dͼ�������ά��
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	int *blk_idx,
	dtype *finalErr)
{
	int iter_flag,ret;			//�����ı�־
	bool solved;
	int notgood_cnt = 0;
	int good_iters = 0;
	int nu = 2;

	dtype *ex;				//ͶӰ���,(eij_x,eij_y)Ϊ3D��i�����j�ļ���ֵ�����ֵ֮������
	dtype *Jx1,*Jx2;				//J*x�Ľ��
	dtype *g;				//��ż����g
	//dtype *Bg;				//���B*g�Ľ��
	dtype *P_U;				//������·��  P_U=[gt*g/(gt*B*g)]g
	dtype *P_B;				//GN·��, P_B=(B^-1)g, ͬʱҲ��������DOGLEG·��
	dtype *P;
	dtype *UVdiag;
	//dtype *camsParas;		// nCams*(3+3)

	dtype ex_L2;			//ͶӰ����L2����
	dtype pred_ex_L2;		//���Ӳ�����ģ��Ԥ���ͶӰ����L2����
	dtype act_ex_L2;		//���Ӳ�����ʵ�ʵ�ͶӰ����L2����
	dtype gtBg;				//g^t*B*g
	dtype gtg;
	dtype dk;				//��k�ε�����Trust-region�뾶
	dtype lambda;
	dtype g_norm;
	dtype p_norm;
	dtype origin_lambda = 0.0;


	int nTotalParas = nCams*cnp + n3Dpts*pnp;
	int nCamsParas = nCams*cnp;
	int nPts3dParas = n3Dpts*pnp;
	int AblkSize = mnp*cnp;
	int BblkSize = mnp*pnp;

	dk = 1;
	lambda = 0;
	//*******************************************************
	UVdiag = (dtype*)emalloc(nTotalParas * sizeof(dtype));
	ex = (dtype*)emalloc(mnp*n2Dprojs * sizeof(dtype));
	Jx1 = (dtype*)emalloc(mnp*(nCams*n3Dpts) * sizeof(dtype));
	Jx2= (dtype*)emalloc(mnp*(nCams*n3Dpts) * sizeof(dtype));
	g = (dtype*)emalloc(nTotalParas*sizeof(dtype));
	P_U = (dtype*)emalloc(nTotalParas*sizeof(dtype));
	P_B = (dtype*)emalloc(nTotalParas*sizeof(dtype));
	P = (dtype*)emalloc(nTotalParas * sizeof(dtype));
	//********************************************************

	//����ͶӰ���ex,����L2����
	cur_tm = clock();
	compute_exQT(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, psbaPtr->cams_buffer, psbaPtr->pts3D_buffer, ex);
	ex_L2 = compute_L2_sq(n2Dprojs*mnp, ex);
	//initErr = ex_L2;
	ex_tm = clock() - cur_tm;

	iter_flag = ITER_CONTINUE;
	for (; itno < 50; itno++)
	{
		//printf("***** iter %d *****\n", itno);
		//***** ����J����(Jacobian)
		cur_tm = clock();
		compute_jacobiQT(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, NULL, NULL);
		jac_tm += (clock() - cur_tm);

		//***** ����g=J^t*ex
		cur_tm=clock();
		compute_g(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, -2, g);
		g_tm += (clock() - cur_tm);
		//***** ����õ�pU
		compute_Jmultiply(psbaPtr, mnp, n3Dpts, nCams, n2Dprojs, psbaPtr->g_buffer, Jx1);
		gtBg =2*dotProduct(Jx1, Jx1, mnp*(nCams*n3Dpts));		//B=2*J^t*J
		gtg = dotProduct(g, g, nTotalParas);
		for (int i = 0; i < nTotalParas; i++) {
			P_U[i] = -(g[i] * gtg) / gtBg;
		}
		//����S����
		cur_tm = clock();
		compute_U(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, 2, NULL);			//tested
		//printBuf2D_blk(stdout, psbaPtr->queue, psbaPtr->U_buffer, 6 * 6 * 58, 6, 6, "U");
		compute_V(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, 2, NULL);			//tested
		//printBuf1D(debug_file, psbaPtr->queue, psbaPtr->UVdiag_buffer,0, nTotalParas, "UVdiag");
		compute_Wblks(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, NULL, NULL, 2, NULL);		//tested
		S_tm += (clock() - cur_tm);
		//*****����õ�pB
		solved = false;
		while (!solved) {
			solved = compute_PB(psbaPtr, cnp, pnp, mnp, nCams, n3Dpts, n2Dprojs, NULL, NULL, blk_idx, &lambda, P_B, UVdiag);
			if (!solved) {
				printf("chol failed.\n");
				if (origin_lambda != 0.0) {
					if (nu > 4) {
						iter_flag = ITER_TURN_TO_LM; *finalErr = ex_L2;
						return iter_flag;
					}
					else {
						lambda = lambda*nu;
						nu = nu * 2;
						restore_UVdiag(psbaPtr, cnp, pnp, n3Dpts, nCams);
					}
				}
				else
					restore_UVdiag(psbaPtr, cnp, pnp, n3Dpts, nCams);
			}
			else {//solved
				nu = 2;
				origin_lambda = lambda;
			}
		}
		
		//*****����J*pU, J*pB
		clEnqueueWriteBuffer(psbaPtr->queue, psbaPtr->dp_buffer, CL_TRUE, 0,
			nTotalParas * sizeof(dtype), P_U, 0, NULL, NULL);
		compute_Jmultiply(psbaPtr, mnp, n3Dpts, nCams, n2Dprojs, psbaPtr->dp_buffer, Jx1);
		clEnqueueWriteBuffer(psbaPtr->queue, psbaPtr->dp_buffer, CL_TRUE, 0,
			nTotalParas * sizeof(dtype), P_B, 0, NULL, NULL);
		compute_Jmultiply(psbaPtr, mnp, n3Dpts, nCams, n2Dprojs, psbaPtr->dp_buffer, Jx2);

		dtype pUtBpU, pUtBpB, pBtBpB;
		pUtBpU = 2*dotProduct(Jx1, Jx1, mnp*n3Dpts*nCams);
		pUtBpB = 2*dotProduct(Jx1, Jx2, mnp*n3Dpts*nCams);
		pBtBpB = 2 * dotProduct(Jx2, Jx2, mnp*n3Dpts*nCams);

		iter_flag = ITER_CONTINUE;
		///////////////////////////////////////////////
		while (iter_flag == ITER_CONTINUE) {
			p_norm=compute_p_2(nTotalParas, pUtBpU, pUtBpB, pBtBpB, dk, P_U, P_B, P, g);

			//ʹ��P_B���²���,�õ�new_p,��Ϊcams_data��pts3D_data
			clEnqueueWriteBuffer(psbaPtr->queue, psbaPtr->dp_buffer, CL_TRUE, 0,
				nTotalParas*sizeof(dtype), P, 0, NULL, NULL);
			//printBuf1D(stdout, psbaPtr->queue, psbaPtr->dp_buffer, nTotalParas, "dp");
			compute_newp(psbaPtr, nCams*cnp, n3Dpts*pnp, NULL);
			//printBuf1D(stdout, psbaPtr->queue, psbaPtr->newCams_buffer, nCamsParas, "cams_paras[after]");
			//printBuf1D(stdout, psbaPtr->queue, psbaPtr->newPts3D_buffer, nPts3dParas, "pts3D_paras[after]");
			//****** ʹ��new_p����ʵ��ͶӰ���
			cur_tm = clock();
			compute_exQT(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,
				psbaPtr->newCams_buffer, psbaPtr->newPts3D_buffer, ex);
			act_ex_L2 = compute_L2_sq(n2Dprojs*mnp, ex);
			ex_tm += (clock() - cur_tm);

			if (abs((ex_L2 - act_ex_L2)/ex_L2) < PSBA_EPSILON2)
			{
				//printf("step is small,exit.\n");
				iter_flag = ITER_DP_NO_CHANGE;
				break;	// break while (iter_flag == ITER_CONTINUE)
			}

			//****** ����new_p����ģ�͵�ͶӰ���
			//pred_ex_L2
			//L(p+dp)=L(p)-2*g*dp+dp^t*J^t*J*dp
			dtype Jx_norm;
			cur_tm = clock();
			compute_Jmultiply(psbaPtr, mnp, n3Dpts, nCams, n2Dprojs, psbaPtr->dp_buffer, Jx1);
			Jx_norm = 2*compute_L2_sq(mnp*nCams*n3Dpts, Jx1);  //dpt*B*dp
			pred_ex_L2 = dotProduct(g, P, nTotalParas);
			pred_ex_L2 += ex_L2+ Jx_norm/2;
			pred_ex_tm += (clock() - cur_tm);

			//����radius
			dtype radius;
			radius = 1 / (Jx_norm / (p_norm*p_norm));
			radius = sqrt(2 * 3.1415926 * radius);

			//***** ����rho=(ex_L2-act_ex_L2)/(ex_L2-pred_ex_L2)
			dtype rho;
			rho = (ex_L2 - act_ex_L2) / (ex_L2 - pred_ex_L2);
			if (rho < (1.0 / 4.0) || act_ex_L2>ex_L2)
			{
				dk = dk / 4;		//��������İ뾶
				printf("iter %d reduce region\n", itno);
			}
			else if (rho >=(3.0/4.0) && act_ex_L2<ex_L2)
			{
				iter_flag = ITER_PASS;
				update_p(psbaPtr, nCamsParas, nPts3dParas, NULL);
				*finalErr = act_ex_L2;
				//ģ�����㹻������ʵ�ʺ�������չ����
				dk = fmin(2 * dk, MAX_DELTA);
				//if (radius < 0.5) lambda = fmax(radius,1e-3);
				//else lambda = origin_lambda;
			}
			else if(rho>=(1.0/4.0) && rho < (3.0/4.0) && act_ex_L2<ex_L2)
			{ 
				//���ܱ��ε����Ľ��, ���²���
				iter_flag = ITER_PASS;
				update_p(psbaPtr, nCamsParas, nPts3dParas,NULL);
				*finalErr = act_ex_L2;
			}
			else if (isnan(rho))
			{
				iter_flag = ITER_TURN_TO_LM; *finalErr = ex_L2;
				return iter_flag;
			}
			printf("itno=%d\tErr:%.15E\tDelta=%f\tRho=%f\tnorm_p=%f\tLambda=%E\n",itno, act_ex_L2, dk, rho, p_norm, lambda);
			//stop test
			if (abs((act_ex_L2 - ex_L2) / ex_L2) <= PSBA_EPSILON2) {
				iter_flag = ITER_ERR_SMALL_ENOUGH;
				break;
			}
			//nogood iter test
			if (rho < 1.0 / 4) {
				notgood_cnt++;
				if (notgood_cnt >= 5) {
					iter_flag = ITER_TURN_TO_LM;
					break;
				}
			}
			else
				notgood_cnt = 0;
			//good iter test
			if (rho > 3.0 / 4 && act_ex_L2 < ex_L2) {
				good_iters++;
				if (good_iters >= 10) { lambda = 0.0; origin_lambda = 0.0; good_iters = 0; }
			}
			else
				good_iters = 0;
			if (rho > (1.0 / 4) && act_ex_L2 < ex_L2)
			{
				ex_L2 = act_ex_L2;
			}


		} //end of while (iter_flag == ITER_CONTINUE)
		if (iter_flag != ITER_PASS)
			break;
	}//end of for (itno = 0; itno < 50; itno++)

	free(UVdiag); free(ex); free(Jx1); free(Jx2);
	free(g); free(P_U); free(P_B); free(P);

	return iter_flag;
}

#include "cl_linearalg.h"

bool compute_PB(PSBA_structPtr psbaPtr, int cnp, int pnp, int mnp,
	int nCams, int n3Dpts, int n2Dprojs,
	int *iidx, int *jidx, int *blk_idx, dtype *lambda,
	dtype *P_B, dtype *UVdiag)
{
	int ret;
	int nCamsParas, nTotalParas;
	dtype sum, res;

	nCamsParas = nCams*cnp;	//size of S
	nTotalParas = nCamsParas + pnp*n3Dpts;

	//��U,V�ĶԽ����ϵ���lambda
	cur_tm = clock();
	update_UV(psbaPtr, cnp, pnp, n3Dpts, nCams, *lambda, NULL, NULL);

	//***** ����S��������Ҫ����V^-1, Ȼ�����Y_ij=W*V^-1,����ڼ���S
	///////////////////////////////////////////////////////////////
	//printBuf2D_blk(stdout, psbaPtr->queue, psbaPtr->V_buffer, 61841 * 3 * 3, 3, 3, "V(before)");
	res=compute_Vinv(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, NULL);		//tested

	//printBuf1D(stdout, psbaPtr->queue, psbaPtr->ret_buffer, 0, 1, "ret");
	//printBuf2D_blk(stdout, psbaPtr->queue, psbaPtr->V_buffer, 61841 * 3 * 3, 3, 3, "V(after)");

	compute_Yblks(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, iidx, jidx, NULL);		//tested
	/*
	for (int i = 0; i < n3Dpts; i++)
	{
		int idx = blk_idx[i*nCams + 58];
		if (idx < 0)continue;
		printBuf2D_blk(debug_file, psbaPtr->queue, psbaPtr->Y_buffer, idx * 6 * 3, 6, 3, "Y");
	}
	*/

	compute_S(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, NULL);
	S_tm += (clock() - cur_tm);
	//printBuf2D(debug_file, psbaPtr->queue, psbaPtr->S_buffer,0, nCamsParas,nCamsParas, "S:");
	///////////////////////////////////////////////////////////////

	//*****�ȱ���S,�ټ���S^-1
	clEnqueueCopyBuffer(psbaPtr->queue, psbaPtr->S_buffer, psbaPtr->Saux_buffer, 0, 0,
		sizeof(dtype)*nCamsParas*nCamsParas,
		0, NULL, NULL);
	cur_tm = clock();
	ret = SPDinv(psbaPtr->queue, psbaPtr->kern_cholesky, psbaPtr->kern_trigMat_inv, psbaPtr->kern_trigMat_mul,
		psbaPtr->S_buffer, psbaPtr->diagBlkAux_buffer, psbaPtr->ret_buffer, nCamsParas, NULL);
	Sinv_cnt++;
	Sinv_tm += (clock() - cur_tm);

	if (ret != 0.0)
	{
		if (*lambda == 0.0)
		{
			clEnqueueCopyBuffer(psbaPtr->queue, psbaPtr->Saux_buffer, psbaPtr->S_buffer, 0, 0,
				sizeof(dtype)*nCamsParas*nCamsParas, 0, NULL, NULL);
			//printBuf2D(debug_file, psbaPtr->queue, psbaPtr->S_buffer,0, nCamsParas, nCamsParas, "S:");

			//���ֽܷ⣬����modified cholesky�ô��Խ�������E
			cholmod_blk(psbaPtr->queue,psbaPtr->kern_cholmod_blk,psbaPtr->kern_mat_max, psbaPtr->S_buffer,
				psbaPtr->blkBackup_buffer, psbaPtr->diagBlkAux_buffer, psbaPtr->E_buffer, psbaPtr->ret_buffer, nCamsParas, NULL);

			//printBuf1D(stdout, psbaPtr->queue, psbaPtr->E_buffer,0, nCamsParas, "diag(after modified cholesky):");
			//printBuf2D(stdout, psbaPtr->queue, psbaPtr->S_buffer, camsParas, camsParas, "L:");
			compute_cholmod_E(psbaPtr->queue, psbaPtr->kern_cholmod_E, psbaPtr->S_buffer, psbaPtr->E_buffer, nCamsParas, P_B);
			//printBuf1D(stdout, psbaPtr->queue, psbaPtr->E_buffer,0, nCamsParas, "E:");
			//����lambda
			sum = 0.0;
			for (int i = 0; i < nCamsParas; i++) {
				//if (isnan(P_B[i]))
				//	printf("xx\n");
				sum += P_B[i];
			}
			*lambda =fabs(sum) / nCamsParas;
			//*lambda = 2.1256e-4;
			return false;
		}
		else {
			*lambda = 2 * (*lambda);
			return false;
		}
	}
	//printBuf2D(stdout, psbaPtr->queue, psbaPtr->S_buffer, camsParas, camsParas, "Sinv:");

	//***** ����Gauss-Newton����P_B==(B^-1)g=[dpa; dpb]
	///////////////////////////////////////////////////////
	//***** ����epa=ea-Y*eb
	compute_ea(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, NULL);

	//***** ���dpa=(S^-1)*epa
	//matVec_mul(psbaPtr, nCams*cnp, nCams*cnp, psbaPtr->S_buffer, psbaPtr->eab_buffer, psbaPtr->dp_buffer, NULL);
	matVec_mul(psbaPtr->queue, psbaPtr->kern_matVec_mul,
		psbaPtr->S_buffer, psbaPtr->eab_buffer, psbaPtr->dp_buffer, nCams*cnp, nCams*cnp, NULL);

	//printBuf1D(stdout, psbaPtr->queue, psbaPtr->dp_buffer, nCams*cnp, "dpa");

	//***** ����epb, epb=epb-(W^t)*dpa
	compute_eb(psbaPtr, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, NULL);

	//***** ���dpb=(V^-1)*eb, ��ΪVinv�ǿ���󣬲���ʹ��compute_matvecmul
	compute_dpb(psbaPtr, cnp, pnp, nCams, n3Dpts, P_B);
	//printBuf1D(stdout, psbaPtr->queue, psbaPtr->dp_buffer, totalParas, "dp");
	///////////////////////////////////////////////////////
	for (int i = 0; i < nTotalParas; i++)
		P_B[i] = -P_B[i];

	/*
	printf("P_B:\n");
	for (int i = 0;i < totalParas; i++)
		printf("%le  ", P_B[i]);
	printf("\n");
	*/

	return true;
}



/*
����dog-leg�㷨�е�tauֵ
tau<0��ʾ����
��������P_B��
*/
inline dtype compute_DG_path(dtype *P_U, dtype *P_B, int size, dtype dk)
{
	dtype norm_PU, norm_PB, a, b, c, Ai, Bi, tau1;

	norm_PU = compute_L2_sq(size, P_U);
	norm_PU = sqrt(norm_PU);

	norm_PB = compute_L2_sq(size, P_B);
	norm_PB = sqrt(norm_PB);

	//P=dk*p_U/norm(p_U)
	if (norm_PU >= dk)
	{
		for (int i = 0; i < size; i++)
			P_B[i] = dk*P_U[i] / norm_PU;
	}
	else {
		if (norm_PB >= dk)
		{
			//����tau
			a = 0.0; b = 0.0; c = 0.0;
			for (int i = 0; i < size; i++)
			{
				Ai = P_B[i] - P_U[i];		//Ai
				Bi = 2 * P_U[i] - P_B[i];
				a += Ai*Ai;
				b += Ai *Bi;
				c += Bi*Bi;
			}
			b = 2 * b;
			c = c - dk*dk;
			tau1 = (-b + sqrt(b*b - 4 * a*c)) / (2 * a);

			for (int i = 0; i < size; i++)
			{
				//p=p_U+(tau-1)*(p_B-p_U);
				a = P_B[i] - P_U[i];
				b = (tau1 - 1)*a;
				c = P_U[i] + b;
				P_B[i] = P_U[i] + (tau1 - 1)*(P_B[i] - P_U[i]);
			}
		}

		//if norm_PB<dk, then p=P_B
	}

	return 0;
}


dtype compute_p(int nTotalParas, dtype pUtBpU, dtype pUtBpB, dtype pBtBpB, dtype delta,
	dtype *P_U,dtype *P_B, dtype *p, dtype *g)
{
	dtype F00, F01, F10, F11;
	dtype G00, G01, G10, G11;
	dtype pUpU, pUpB,pBpB, pUg, pBg;
	dtype eta1, eta2, p_norm;

	F00 = pUtBpU; F01 = pUtBpB; F10 = pUtBpB; F11 = pUtBpU;
	pUpU = 0.0;
	pUg = dotProduct(P_U, g, nTotalParas);
	pBg = dotProduct(P_B, g, nTotalParas);
	pUpU=dotProduct(P_U, P_U, nTotalParas);
	pUpB = dotProduct(P_U, P_B, nTotalParas);
	pBpB = dotProduct(P_B, P_B, nTotalParas);
	
	eta1 = (pBg*pUtBpB) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU)
		- (pBtBpB*pUg) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU);
	eta2 = (pUg*pUtBpB) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU)
		- (pBg*pUtBpU) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU);

	p_norm = 0.0;
	for (int i = 0; i < nTotalParas; i++) {
		p[i] = eta1*P_U[i] + eta2*P_B[i];
		p_norm += p[i] * p[i];
	}
	p_norm = sqrt(p_norm);

	if (p_norm > delta)
	{
		pUtBpU -= pUpU;
		pUtBpB -= pUpB;
		pBtBpB -= pBpB;

		eta1 = (pBg*pUtBpB) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU)
			- (pBtBpB*pUg) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU);
		eta2 = (pUg*pUtBpB) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU)
			- (pBg*pUtBpU) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU);

		p_norm = 0.0;
		for (int i = 0; i < nTotalParas; i++) {
			p[i] = eta1*P_U[i] + eta2*P_B[i];
			p_norm += p[i] * p[i];
		}
		p_norm = sqrt(p_norm);
		if (p_norm > delta)
		{
			for (int i = 0; i < nTotalParas; i++)
				p[i] = delta*p[i] / p_norm;
			return delta;
		}
	}
	return p_norm;

}

dtype compute_p_2(int nTotalParas, dtype pUtBpU, dtype pUtBpB, dtype pBtBpB, dtype delta,
	dtype *P_U, dtype *P_B, dtype *p, dtype *g)
{
	dtype F00, F01, F10, F11;
	dtype G00, G01, G10, G11;
	dtype pUpU, pUpB, pBpB, pUg, pBg;
	dtype eta1, eta2, p_norm, pU_norm, pB_norm;

	F00 = pUtBpU; F01 = pUtBpB; F10 = pUtBpB; F11 = pUtBpU;
	pUpU = 0.0;
	pUg = dotProduct(P_U, g, nTotalParas);
	pBg = dotProduct(P_B, g, nTotalParas);
	pUpU = dotProduct(P_U, P_U, nTotalParas);
	pUpB = dotProduct(P_U, P_B, nTotalParas);
	pBpB = dotProduct(P_B, P_B, nTotalParas);

	eta1 = (pBg*pUtBpB) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU)
		- (pBtBpB*pUg) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU);
	eta2 = (pUg*pUtBpB) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU)
		- (pBg*pUtBpU) / (-pUtBpB*pUtBpB + pBtBpB*pUtBpU);

	p_norm = 0.0;
	for (int i = 0; i < nTotalParas; i++) {
		p[i] = eta1*P_U[i] + eta2*P_B[i];
		p_norm += p[i] * p[i];
	}
	p_norm = sqrt(p_norm);

	if (p_norm > delta)
	{
		//����norm(pU),norm(pB)
		pU_norm = 0.0; pB_norm = 0.0;
		for (int i = 0; i < nTotalParas; i++) {
			pU_norm += P_U[i] * P_U[i];
			pB_norm += P_B[i] * P_B[i];
		}
		pU_norm = sqrt(pU_norm);
		pB_norm = sqrt(pB_norm);
		if (pU_norm > delta)
		{
			for (int i = 0; i < nTotalParas; i++)
				p[i] = delta*P_U[i] / pU_norm;
			return delta;
		}
		else if (pB_norm <= delta) {
			for (int i = 0; i < nTotalParas; i++) {
				p[i] = P_B[i];
				p_norm += p[i] * p[i];
			}
			return sqrt(p_norm);
		}
		else {
			//����tau
			dtype a = 0.0; 
			dtype b = 0.0; 
			dtype c = 0.0;
			dtype Ai, Bi, b2_4ac, tau;
			for (int i = 0; i < nTotalParas; i++)
			{
				Ai = P_B[i] - P_U[i];		//Ai
				Bi = 2 * P_U[i] - P_B[i];
				a += Ai*Ai;	b += Ai *Bi; c += Bi*Bi;
			}
			b = 2 * b;		c = c - delta*delta;
			b2_4ac = b*b - 4 * a*c; if (abs(b2_4ac) < 1e-12) b2_4ac = 0;
			tau = (-b + sqrt(b2_4ac)) / (2 * a);

			for (int i = 0; i < nTotalParas; i++)
			{
				p[i] = P_U[i] + (tau - 1)*(P_B[i] - P_U[i]);
			}
			return delta;
		}
	}
	return p_norm;
}