#pragma once

#include "cl_psba.h"
#include "psba.h"


/*
����ͶӰ��� ex
*/
void compute_exQT(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	cl_mem cams_buffer,
	cl_mem pts3D_buffer,
	//���
	dtype *ex				//�洢����ͶӰ��hx_ij-->��i��3D�㵽��j������ϵ�ͶӰ�����֮�������ex_11,ex12,...,ex_nm��˳���š�
);


/*
����jacobi����ʹ��OpenCL
�����ŵ�JA_buffer��JB_buffer
*/
void compute_jacobiQT(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	dtype *jac_A, dtype *jac_B
);

/*
����U����
�����ŵ�U_buffer��
*/
void compute_U(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	dtype coeff,
	dtype *out);

/*
����U����
��������V_buffer
*/
void compute_V(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	dtype coeff,
	dtype *out);

dtype maxElmOfUV(PSBA_structPtr psbaPtr, int totalParas, dtype *UVdiag);

void update_UV(PSBA_structPtr psbaPtr, int cnp, int pnp, int n3Dpts, int nCams, 
	dtype mu, dtype *U, dtype *V);

dtype compute_Vinv(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	dtype *V);

/*
����W����,�����洢��ʽ
��������W_buffer��
*/
void compute_Wblks(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	int *iidx, int *jidx,
	dtype coeff,
	dtype *Wblks);


void compute_Yblks(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	int *iidx, int *jidx,
	dtype *Yblks);


void compute_S(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	dtype *S);

/*
����g����. g=(J^t)*ex
*/
void compute_g(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	dtype coeff,
	dtype *g);

/*
����ea��ֵ��ea=ga-Y*gb
*/
void compute_ea(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	dtype *ea);

/*
����eb��ֵ��eb=gb-(W^t)*dpa
*/
void compute_eb(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp,		//�ֱ����������ά��(12)��3D������ά��(3)��2Dͼ�������ά��(2)
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	dtype *eab);

void compute_dpb(
	//����
	PSBA_structPtr psbaPtr,
	int cnp, int pnp, int nCams, int n3Dpts,
	dtype *dp);		//�����dpb����

void compute_newp(PSBA_structPtr psbaPtr, int nCamParas, int n3DptsParas, dtype *new_p);
void update_p(PSBA_structPtr psbaPtr, int nCamParas, int n3DptsParas, dtype *p);