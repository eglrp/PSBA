#include "stdafx.h"

#include <time.h>

#include "cl_psba.h"
#include "psba.h"
#include "cl_linearalg.h"
#include "misc.h"

extern int itno;
extern FILE *debug_file;


/*
����Mv,�����������ĳ˻�������浽out�С�
rmat_size --�����гߴ�
cmat_size --�����гߴ�
*/
void matVec_mul(cl_command_queue queue, cl_kernel kern_matVec_mul, 
	cl_mem M_buffer,cl_mem v_buffer, cl_mem out_buffer, int mat_rsize, int mat_csize, dtype *out)
{
	cl_int err;

	//ִ���ں�
	//********���м��������Ǿ������**********/
	err = clSetKernelArg(kern_matVec_mul, 0, sizeof(int), &mat_csize);
	err |= clSetKernelArg(kern_matVec_mul, 1, sizeof(cl_mem), &M_buffer);
	err |= clSetKernelArg(kern_matVec_mul, 2, sizeof(cl_mem), &v_buffer);
	err |= clSetKernelArg(kern_matVec_mul, 3, sizeof(cl_mem), &out_buffer);

	size_t global_size[1] = {(size_t) mat_rsize };		//���Ӧ��Ϊ������������Ա�����ڷǷ�������
	err = clEnqueueNDRangeKernel(queue, kern_matVec_mul,
		1, NULL,	//������ʹ�ö�ά�����ռ�
		global_size,
		NULL,	//��OpenCL����
		0, NULL, NULL);
	err = clFinish(queue);
	checkErr(err, __FILE__, __LINE__);

	if (out != NULL)
	{
		err = clEnqueueReadBuffer(queue, out_buffer, CL_TRUE, 0,
			sizeof(dtype)*mat_rsize, out, 0, NULL, NULL);
		checkErr(err, __FILE__, __LINE__);

	}

#if DEBUG_DP==1
	// ��ȡ���
	//err = clEnqueueReadBuffer(psbaPtr->queue, out_buffer, CL_TRUE, 0,
	//	sizeof(dtype)*rmat_size, out, 0, NULL, NULL);
	checkErr(err, __FILE__, __LINE__);
#endif

}
