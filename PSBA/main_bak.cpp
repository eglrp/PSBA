// bundle_adjustment.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include <time.h>
#include <iostream>

#include "psba.h"
#include "readparams.h"
#include "misc.h"
#include "cl_psba.h"


using namespace std;

FILE *debug_file;
extern int itno;

clock_t start_time, end_time,cur_tm;
clock_t tm_diff[30];



char cams_file[] = "..\\data\\54cams.txt";
char pts_file[] = "..\\data\\54pts.txt";

PSBA_struct psbaStruct;		//LM�㷨��cl��Ϣ�ṹ��

//ԭʼ��Kֵ��fu,u0,v0,ar// aspect ratio=ax/ay
dtype K[5] = { 851.57945,851.57945 *1.00169,330.24755,262.19500,0 };	//������ڲ�, a_x,a_y,x0,y0
dtype KK[5] = { 851.57945,330.24755,262.19500,1.00169,0 };

int main()
{
	int cnp_origin=6;					//ԭʼ������Ĳ���ά��(3+3,��Ԫ��������λ��������
	int cnp = 6;				//ת������������ά��(6+3,R and t)
	int pnp = 3;				//3D���ά��
	int mnp=2;					//2DͶӰ���ά��

	int n3Dpts;				//3D�������
	int nCams;					//���������
	int n2Dprojs;			//2DͶӰ�������
	dtype *motion_data;			//ָ��洢�������(3+3,��Ԫ��������λ��������+3D�㣨3���Ŀռ�  
	dtype *initrot;				//ָ��洢�����ʼ��ת��Ԫ���Ŀռ�

	dtype *impts_data;			//ָ��洢2DͶӰ��Ŀռ䡣   ncams*cnp + numpts3D*pnp, 
	//impts_data�洢˳��Ϊ��3D��1�ڶ�Ӧ����ϵ�2DͶӰ�㣬...3D��n�ڶ�Ӧ����ϵ�2DͶӰ��
	dtype *impts_cov;			//2DͶӰ���Э�����ΪNULL
	char *vmask;				//3D����Ը���������룬vmask[i,j]=0��ʾ3D��i�������j����ͼ��㡣�ռ��СnCams*n3Dpts
	dtype *motion2_data;		//�����Ҫת������motion_data�еĲ���ת����浽�˴�
	dtype finalErr;

	motion2_data = NULL;

	errno_t err;
	err = fopen_s(&debug_file,"e:\\psba_debug.txt", "wt");
	if (err != 0) {
		printf("can't open debug file.\n");
		exit(1);
	}


	//1,��ȡ����
	readInitialSBAEstimate(cams_file, pts_file, cnp_origin, pnp, mnp,
		quat2vec, cnp_origin + 1,		//cnp_origin=6,���ļ�������ʹ��4��ֵ����Ԫ����������cnp+1
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
	for (int j = 0; j < nCams; j++)
	{
		printf("[%d]", j);
		for (int k = 0; k < 4; k++)
			printf("%le  ", initrot[j * 4 + k]);
		printf("\n");
	}

	//##########��motion_data���зֽ�Ϊ�������cams_data��3D�����������������Ԫ��ת��Ϊ��ת����
	//if (cnp == 12 && cnp_origin == 6) {
	//	motion2_data = (dtype*)malloc((nCams * cnp +n3Dpts*pnp)* sizeof(dtype));
	//	camsFormatTrans(motion_data, nCams, 3, 3, motion2_data);		//3,3�ֱ��ʾ��Ԫ������v��ά�� ��λ�Ƶ�ά��
	//	memcpy((void*)&motion2_data[nCams*cnp],(void*)&motion_data[nCams*cnp_origin], n3Dpts*pnp * sizeof(dtype));		//����
	//}
	//���motion_data���������rot����
	for (int j = 0;j < nCams; j++) {
		int base = j*cnp;
		motion_data[base++] = 0;
		motion_data[base++] = 0;
		motion_data[base] = 0;
	}
	//######### ����openCL device, �Լ���������Ҫ��buffer ##########//
	setup_cl(&psbaStruct,cnp,pnp,mnp,nCams,n3Dpts,n2Dprojs);
	//  ����ʼ����
	fill_initBuffer(&psbaStruct, cnp,pnp,mnp,nCams,n3Dpts,n2Dprojs,
		K, impts_data, initrot, motion_data);

	//����LM�㷨
	start_time = clock();
	if(cnp==6)
		levmar(&psbaStruct, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs, K, impts_data, vmask,
			initrot, motion_data,&finalErr);
	else
		levmar(&psbaStruct, cnp, pnp, mnp, n3Dpts, nCams, n2Dprojs,K,impts_data, vmask,
			initrot, motion2_data, &finalErr);
	end_time = clock();

	release_buffer(&psbaStruct);
	printf("time eclipse %lf s\n", double(end_time - start_time) / CLOCKS_PER_SEC);
	//�����������
	printf("final error: %le \n",finalErr);
	printf("total iteration: %d\n", itno);

	for (int i = 0; i < 30; i++)
		printf("diff tm %d=%ld \n",i, tm_diff[i] );

	system("pause");
    return 0;
}

