#pragma once

//#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <CL/cl.h>
#include <string>
#include "psba.h"

typedef struct {
	cl_device_id device;
	cl_context context;
	cl_command_queue queue;
	cl_command_queue queue_device;		//queue on device
	cl_program program;
	cl_program program_new;

	//buffer associated to LM
	cl_mem Kparas_buffer;
	cl_mem impts_buffer;
	cl_mem initcams_buffer;
	cl_mem cams_buffer;		//�������(3 + 3, R, t), R����Ϊ��Ԫ����(q1,q2,q3),q0����㡣
	cl_mem pts3D_buffer;	//3D������(X, Y, Z)
	cl_mem vmask_buffer;
	cl_mem iidx_buffer;
	cl_mem jidx_buffer;
	cl_mem comm3DIdx_buffer;		//Ϊÿ��[k,l]����Է���n3Dpts��int,���ڼ�¼[k,l]����Թ�ͬ��3D������
	cl_mem comm3DIdxCnt_buffer;		//���ڼ�¼[k,l]����Թ�ͬ��3D�������

	cl_mem blkIdx_buffer;
	cl_mem JA_buffer;	//���jacobi����J��A_ij�ӿ飬A_ij�ӿ��СΪmnp*cnp��˳��ΪA_11,A_12,...,A_1m,A_21,...,A_nm�����ĳA_ij=0�����䲻ռ�ռ䡣
	cl_mem JB_buffer;
	cl_mem ex_buffer;
	cl_mem Jmul_buffer;	//���J*X�Ľ��, size=totalParas
	cl_mem U_buffer;	//size= cnp*cnp*nCams
	cl_mem V_buffer;	//size= pnp*pnp*nPts3D
	//cl_mem Vinv_buffer;
	cl_mem UVdiag_buffer;	//size=totalParas
	cl_mem W_buffer;		//size=n2Dprojs*cnp*pnp
	cl_mem Y_buffer;		//size=n2Dprojs*cnp*pnp
	cl_mem S_buffer;		
	cl_mem diagBlkAux_buffer;	//S�����nCamsParas*3, 3Ϊcholesky�����ĳߴ�,�����ڴ��Ljj^-1��
	cl_mem blkBackup_buffer;	//>=nCamsParas*3, ����cholmod
	cl_mem E_buffer;		//����modified cholesky���㣬����Խ��ߵ�����ֵ
	cl_mem Saux_buffer;		//��S_buffer�ߴ�һ��
	cl_mem g_buffer;
	cl_mem dp_buffer;
	//cl_mem Q0_buffer;		//��Ԫ����q0��q0=sqrt(1-q1^2-q2^2-q3^3)
	cl_mem Bg_buffer;		//B*b�Ľ��
	cl_mem eab_buffer;
	cl_mem newCams_buffer;
	cl_mem newPts3D_buffer;
	cl_mem ret_buffer;		//dtype

	cl_kernel kern_compute_exQT;
	cl_kernel kern_compute_jacobiQT;
	cl_kernel kern_compute_U;
	cl_kernel kern_compute_V;
	cl_kernel kern_restore_UVdiag;

	cl_kernel kern_compute_Wblks;
	cl_kernel kern_compute_g;
	cl_kernel kern_update_UV;
	cl_kernel kern_compute_Vinv;

	cl_kernel kern_compute_Yblks;
	cl_kernel kern_compute_S;
	cl_kernel kern_compute_ea;
	
	//������kernel����spd matrix inverse
	cl_kernel kern_cholesky;
	cl_kernel kern_trigMat_inv;
	cl_kernel kern_trigMat_mul;

	cl_kernel kern_cholmod_blk;
	cl_kernel kern_mat_max;
	cl_kernel kern_cholmod_E;

	cl_kernel kern_matVec_mul;
	cl_kernel kern_compute_eb;
	cl_kernel kern_compute_dpb;

	cl_kernel kern_compute_newp;
	cl_kernel kern_update_p;

	cl_kernel kern_compute_Bg;
	cl_kernel kern_compute_Jmultiply;


}PSBA_struct,*PSBA_structPtr;



void setup_cl(PSBA_structPtr psbaPtr,int cnp, int pnp, int mnp, int nCams, int n3Dpts, int n2Dprojs);
void release_buffer(PSBA_structPtr psbaPtr);

cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename);
cl_device_id get_first_device();
void checkErr(cl_int err, const char* file, int num);
int convertToString(const char *filename, std::string& s);

void printBuf2D(FILE *file, cl_command_queue queue, cl_mem buf, int offset, int rsize,int csize, char *title);
void printBuf2D_blk(FILE *file, cl_command_queue queue, cl_mem buf, int offset, int rsize, int csize, char *title);

void printBuf1D(FILE *file, cl_command_queue queue, cl_mem buf, int offset, int size, char *title);

void fill_initBuffer(PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp, int nCams, int n3Dpts, int n2Dprojs,
	dtype *K,
	dtype *impts_data,		//2Dͼ�������
	dtype *initcams_data,	//��ʼ�������ת��Ԫ��(4-vec)
	dtype *motion_data		//�������(9+3, R,t)+3D������(X,Y,Z)
);

void fill_initBuffer2(PSBA_structPtr psbaPtr,
	int cnp, int pnp, int mnp, int nCams, int n3Dpts, int n2Dprojs,
	dtype *Kparas,
	dtype *impts_data,		//2Dͼ�������
	dtype *initcams_data,	//��ʼ�������ת��Ԫ��(4-vec)
	dtype *camsExParas,		//�������(9+3, R,t)+
	dtype *pts3Ds			//3D������(X,Y,Z)
);

void fill_idxBuffer(PSBA_structPtr psbaPtr,
	int nCams, int n3Dpts, int n2Dprojs,
	int *comm3DIdx, int *comm3DIdxCnt,
	int *iidx, int *jidx, int *blk_idx);