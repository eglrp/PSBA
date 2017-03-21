// bundle_adjustment.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include <time.h>
#include <iostream>

#include "psba.h"
#include "readparams.h"
#include "misc.h"
#include "cl_psba.h"
#include "levmar.h"
#include "trust_region.h"


using namespace std;

FILE *debug_file;
extern int itno;

clock_t start_time, end_time;

dtype initErr;		//��ʼ���
clock_t cur_tm;
clock_t total_tm=0;	//�㷨��ʱ��
int itno=0;			//��ǰ��������
int jac_cnt=0;			//��¼jac�������
int Sinv_cnt=0;			//��¼Binv�������
clock_t alloc_tm = 0;	//����ռ��ʱ��
clock_t genidx_tm = 0;	//����������ʱ��
clock_t jac_tm = 0;		//��¼jac������ʱ��
clock_t S_tm = 0;		//����S_tm����ʱ��
clock_t Sinv_tm=0;		//��¼Sinv������ʱ��
clock_t ex_tm = 0;		//����ex��ʱ��
clock_t g_tm = 0;		//����g��ʱ��
clock_t pred_ex_tm = 0;	//Ԥ�����ļ���ʱ��


//char cams_file[] = "..\\data\\Trafalgar-50-20431-cams.txt";
//char pts_file[] = "..\\data\\Trafalgar-50-20431-pts.txt";

//char cams_file[] = "..\\data\\Dubrovnik-88-64298-cams.txt";
//char pts_file[] = "..\\data\\Dubrovnik-88-64298-pts.txt";

//char cams_file[] = "..\\data\\Rome-93-61203-cams.txt";
//char pts_file[] = "..\\data\\Rome-93-61203-pts.txt";

//char cams_file[] = "..\\data\\Trafalgar-21-11315-cams.txt";
//char pts_file[] = "..\\data\\Trafalgar-21-11315-pts.txt";

//char cams_file[] = "..\\data\\Venice-52-64053-cams.txt";
//char pts_file[] = "..\\data\\Venice-52-64053-pts.txt";

//char cams_file[] = "..\\data\\Ladybug-138-19878-cams.txt";
//char pts_file[] = "..\\data\\Ladybug-138-19878-pts.txt";

//char cams_file[] = "..\\data\\Dubrovnik-16-22106-cams.txt";
//char pts_file[] = "..\\data\\Dubrovnik-16-22106-pts.txt";

char cams_file[] = "..\\data\\Trafalgar-21-11315-cams.txt";
char pts_file[] = "..\\data\\Trafalgar-21-11315-pts.txt";

//char cams_file[] = "..\\data\\54camsvarK.txt";
//char pts_file[] = "..\\data\\54pts.txt";


PSBA_struct psbaStruct;		//LM�㷨��cl��Ϣ�ṹ��

int main()
{
	//int cnp_origin=6;					//ԭʼ������Ĳ���ά��(3+3,��Ԫ��������λ��������
	int origin_cnp = 11;				//ת������������ά��(K,Q and t)
	int pnp = 3;				//3D���ά��
	int mnp=2;					//2DͶӰ���ά��

	int n3Dpts;				//3D�������
	int nCams;					//���������
	int n2Dprojs;			//2DͶӰ�������
	dtype *motion_data;			//ǰnCams*cnpԪ�ش洢�������(5+3+3,�ڲΣ���Ԫ��������λ������������n3Dpts*pnpԪ�ش洢3D��
	dtype *initrot;				//ָ��洢�����ʼ��ת��Ԫ���Ŀռ�
	dtype *impts_data;			//ָ��洢2DͶӰ��Ŀռ䡣 n2Dprojs*mnp, 
	//impts_data�洢˳��Ϊ��3D��1�ڶ�Ӧ����ϵ�2DͶӰ�㣬...3D��n�ڶ�Ӧ����ϵ�2DͶӰ��
	dtype *impts_cov;			//2DͶӰ���Э�����ΪNULL
	char *vmask;				//3D����Ը���������룬vmask[i,j]=0��ʾ3D��i�������j����ͼ��㡣�ռ��СnCams*n3Dpts
	dtype finalErr;
	dtype *Kparas;				//����������
	dtype *camsExParas;			//�����Σ�Q+t, Q��Q0���������յ���ת

	errno_t err;
	err = fopen_s(&debug_file,"e:\\psba_debug.txt", "wt");
	if (err != 0) {
		printf("can't open debug file.\n");
		exit(1);
	}

	//1,��ȡ����
	//if (readNumParams(cams_file) - 1 == cnp + 5) {
	//	cnp = cnp + 5;
	//}

	readInitialSBAEstimate(cams_file, pts_file, origin_cnp, pnp, mnp,
		quat2vec, origin_cnp + 1,
		&nCams, &n3Dpts, &n2Dprojs,
		&motion_data,
		&initrot,	//�Ὣ����ĳ�ʼ����ת��Ԫ���浽����
		&impts_data, &impts_cov, &vmask
		);
	cout << "���������" << nCams << endl;
	cout << "3D��������" << n3Dpts << endl;
	cout << "2DͶӰ�������:" << n2Dprojs << endl;
	if (impts_cov == NULL)
		cout << "û��ͼ����Э�������" << endl;
	cout << "��ʼ�����Ԫ����" << endl;

	/*
	dtype tmp;
	printf("init rotation:\n");
	for (int j = 0; j < nCams; j++)
	{
		printf("[%d]", j);
		for (int k = 0; k < 4; k++) {
			tmp = initrot[j * 4 + k];
			printf("%f  ", tmp);
		}
		printf("\n");
	}
	*/

	//���motion_data���������rot����
	for (int j = 0;j < nCams; j++) {
		int base = (j+1)*origin_cnp;
		motion_data[base-4] = 0;
		motion_data[base-5] = 0;
		motion_data[base-6] = 0;
	}

	//��������
	//���ڲδ�motion_data�з���,ʣ���Q,tת�Ƶ�motstruct����Ϊ���Ż��Ĳ�����
	int final_cnp = origin_cnp - 5;
	Kparas = (dtype*)emalloc(sizeof(dtype)*nCams * 5);
	camsExParas = (dtype*)emalloc(sizeof(dtype)*nCams*final_cnp);
	for (int j = 0; j < nCams; j++)
	{
		for(int k=0; k<5; k++)
			Kparas[j*5+k] = motion_data[j*origin_cnp +k];
		for (int k = 0; k < final_cnp; k++)
			camsExParas[j*final_cnp +k] = motion_data[j*origin_cnp +k+5];
	}

	/***********************************************/
	/*
	printf("cams K:\n");
	for (int j = 0; j < nCams; j++)
	{
		printf("[%d]", j);
		for (int k = 0; k < 5; k++) {
			tmp = Kparas[j * 5 + k];
			printf("%f  ", tmp);
		}
		printf("\n");
	}
	printf("cams Ex paras:\n");
	for (int j = 0; j < nCams; j++)
	{
		printf("[%d]", j);
		for (int k = 0; k < final_cnp; k++) {
			tmp = camsExParas[j * final_cnp + k];
			printf("%f  ", tmp);
		}
		printf("\n");
	}
	*/
	/***********************************************/

	//######### ����openCL device, �Լ���������Ҫ��buffer ##########//
	setup_cl(&psbaStruct, final_cnp,pnp,mnp,nCams,n3Dpts,n2Dprojs);
	//  ����ʼ����
	fill_initBuffer2(&psbaStruct, final_cnp,pnp,mnp,nCams,n3Dpts,n2Dprojs,
		Kparas, impts_data, initrot, camsExParas,&motion_data[nCams*origin_cnp]);
	
	//����������������buffer
	int *iidx = (int*)emalloc(n2Dprojs * sizeof(int));
	int *jidx = (int*)emalloc(n2Dprojs * sizeof(int));
	int *blk_idx = (int*)emalloc(n3Dpts*nCams * sizeof(int));
	int *comm3DIdx = (int*)emalloc(n3Dpts*nCams*nCams * sizeof(int));
	int *comm3DIdxCnt = (int*)emalloc(nCams*nCams * sizeof(int));
	generate_idxs(nCams, n3Dpts, n2Dprojs, impts_data, vmask, comm3DIdx, comm3DIdxCnt, iidx, jidx, blk_idx);
	fill_idxBuffer(&psbaStruct, nCams, n3Dpts, n2Dprojs, comm3DIdx, comm3DIdxCnt, iidx, jidx, blk_idx);

	int iter_flag;
	start_time = clock();
	while (true) {
		//����LM�㷨
		///*

		iter_flag = levmar(&psbaStruct, final_cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, &finalErr);
		if (iter_flag != ITER_TURN_TO_TR)	
			break;
		//*/

		//����TR�㷨
		///*
		iter_flag = trust_region(&psbaStruct, final_cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, blk_idx, &finalErr);
		if (iter_flag != ITER_TURN_TO_LM)
			break;
		//*/
	}
	end_time = clock();
	printf("iter_flag=%d\n", iter_flag);


	//release_buffer(&psbaStruct);
	printf("time eclipse %lf s\n", double(end_time - start_time) / CLOCKS_PER_SEC);
	//�����������
	printf("initial error: %.15E \n", sqrt(initErr)/n2Dprojs);
	printf("final error: %.15E \n",sqrt(finalErr)/n2Dprojs);
	printf("total iteration: %d\n", itno);
	
	printf("alloc mem tm:\t\t%f\n", alloc_tm / (double)CLOCKS_PER_SEC);
	printf("gen idx tm:\t\t%f\n", genidx_tm / (double)CLOCKS_PER_SEC);
	printf("jac tm:\t\t%lf s\n", double(jac_tm)/ CLOCKS_PER_SEC);
	printf("ex tm tm:\t\t%f\n", ex_tm / (double)CLOCKS_PER_SEC);
	printf("g tm tm:\t\t%f\n", g_tm / (double)CLOCKS_PER_SEC);
	printf("S tm tm:\t\t%f\n", S_tm / (double)CLOCKS_PER_SEC);
	printf("Sinv tm:\t\t%f \t\t cnt:%d\n", double(Sinv_tm) / CLOCKS_PER_SEC, Sinv_cnt);
	printf("pred ex tm:\t\t%f\n", pred_ex_tm / (double)CLOCKS_PER_SEC);

	system("pause");
    return 0;
}

