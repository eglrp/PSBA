/*

���ļ����ݰ������SPD��������������cholesky�ֽ�����Ǿ�������

*/


#include "stdafx.h"

#include <time.h>

#include "cl_psba.h"
#include "psba.h"
#include "cl_spdinv.h"



dtype SPDinv(cl_command_queue queue, cl_kernel kern_cholesky,cl_kernel kern_trigMat_inv,cl_kernel kern_trigMat_mul,
	cl_mem matBuf, cl_mem diagAuxBuf, cl_mem retBuf,int matSize, dtype *outMat)
{
	cl_int err;
	dtype ret = 0.0;

	//�ȶ�S���б���
	//clEnqueueCopyBuffer(queue, S_buffer, psbaPtr->Saux_buffer, 0, 0,
	//	sizeof(dtype)*matSize*matSize,
	//	0, NULL, NULL);

	//��S���зֽ⡣
	ret = cholesky(queue, kern_cholesky,
		matBuf, diagAuxBuf, retBuf, matSize, outMat);

	if (ret != 0.0) return ret;
	//�õ������Ǿ���������,�������Saux_buffer���������С�
	ret = trigMat_inv(queue, kern_trigMat_inv, matBuf, diagAuxBuf, retBuf,
		matSize, outMat);

	trigMat_mul(queue, kern_trigMat_mul, matBuf, diagAuxBuf, matSize, outMat);

}


/******************************************************************************************************/

/*
cholesky�ֽ�
���ԣ�
�ֿ鲢�з�����enqueue on device
���룺
matBuf--���ֽⷽ��, matSize*matSize
diagAuxBuf--����Buffer, >=3*matSize
�����
matBuf--�������Ǵ�ŷֽ���
diagAuxBuf--���Ljj^-1�ӿ�
outMat!=NULL �򽫷ֽ���������outMat
*/
dtype cholesky(cl_command_queue queue, cl_kernel kern_cholesky,
	cl_mem matBuf, cl_mem diagAuxBuf, cl_mem retBuf,
	int matSize, dtype *outMat)
{
	cl_int err;
	dtype ret;

	//set kernel arguments
	err = clSetKernelArg(kern_cholesky, 0, sizeof(cl_mem), &matBuf);
	err = clSetKernelArg(kern_cholesky, 1, sizeof(cl_mem), &diagAuxBuf);
	err = clSetKernelArg(kern_cholesky, 2, sizeof(cl_mem), &retBuf);
	err = clSetKernelArg(kern_cholesky, 3, sizeof(dtype) * 9, NULL);		//T_ii��
	err = clSetKernelArg(kern_cholesky, 4, sizeof(dtype) * 6, NULL);		//L_ii,ֻ�洢������
	err = clSetKernelArg(kern_cholesky, 5, sizeof(int), &matSize);

	//ִ���ں�
	int j = 0;
	size_t global_size[2] = { matSize,3 };
	size_t local_size[2] = { 3,3 };
	err = clSetKernelArg(kern_cholesky, 6, sizeof(int), &j);		//�����ĵ�j��
	err = clEnqueueNDRangeKernel(queue, kern_cholesky,
		2, NULL,	//2D work space
		global_size,
		local_size,
		0, NULL, NULL);
	checkErr(err, __FILE__, __LINE__);
	clFinish(queue);

	err = clEnqueueReadBuffer(queue, retBuf, CL_TRUE, 0,
		sizeof(ret), &ret, 0, NULL, NULL);
	checkErr(err, __FILE__, __LINE__);

	// ��ȡ���
	if (outMat != NULL)
	{
		err = clEnqueueReadBuffer(queue, matBuf, CL_TRUE, 0,
			sizeof(dtype)*matSize*matSize, outMat, 0, NULL, NULL);
		checkErr(err, __FILE__, __LINE__);
	}

#if DEBUG_CHOLESKY==1
	printBuf2D(stdout, queue, matBuf, matSize, matSize, "L(cholesky):");
	printBuf2D(stdout, queue, diagAuxBuf, 3, matSize, "diagInv:");

#endif
	return ret;
}




/*
��������Ǿ���������
���ԣ�
ʹ��3x3�Ŀ飬
ʹ��opencl 2.0��QUEUE_ON_DEVICE����
���룺
matBufΪ�����Ǿ���
auxBufΪ3x3�Խǿ�������
�����
����ֵ��=0��ʾ������ȷ
������buf_spd_A�������ǣ������Խǿ飩�������ǲ�������
*/
dtype trigMat_inv(cl_command_queue queue, cl_kernel kern_trigMat_inv, 
	cl_mem matBuf, cl_mem auxBuf,cl_mem retBuf, int mat_size, dtype *outMat)
{
	cl_command_queue queue_device;
	cl_int err;
	dtype ret = 0.0;
	//int blk_mat_size;

	//********���ò���**********/
	int j = 0;
	err = clSetKernelArg(kern_trigMat_inv, 0, sizeof(cl_mem), &matBuf);
	err |= clSetKernelArg(kern_trigMat_inv, 1, sizeof(cl_mem), &auxBuf);
	err |= clSetKernelArg(kern_trigMat_inv, 2, sizeof(cl_mem), &retBuf);
	err |= clSetKernelArg(kern_trigMat_inv, 3, sizeof(int), &mat_size);
	err |= clSetKernelArg(kern_trigMat_inv, 4, sizeof(int), &j);

	//ִ���ں�,
	size_t global_size[2] = { mat_size,3 };
	size_t local_size[2] = { 3,3 };
	err = clEnqueueNDRangeKernel(queue, kern_trigMat_inv,
		2, NULL,	//2D work space
		global_size,
		local_size,	//local_size,
		0, NULL, NULL);
	checkErr(err, __FILE__, __LINE__);
	clFinish(queue);

	err = clEnqueueReadBuffer(queue, retBuf, CL_TRUE, 0,
		sizeof(ret), &ret, 0, NULL, NULL);
	checkErr(err, __FILE__, __LINE__);

	if (outMat != NULL)
	{
		err = clEnqueueReadBuffer(queue, matBuf, CL_TRUE, 0,
			sizeof(dtype)*mat_size*mat_size, outMat, 0, NULL, NULL);
		checkErr(err, __FILE__, __LINE__);
	}

#if DEBUG_TRIGMAT_INV==1
	printBuf2D(stdout, queue, matBuf, mat_size, mat_size, "Linv:");
#endif
	return ret;
}

/*
����M_inv=(L^-t)*L^-1
buf_spd_A���������Ѿ���L^-t

*/
void trigMat_mul(cl_command_queue queue, cl_kernel kern_trigMat_mul, cl_mem matBuf, 
	cl_mem auxBuf,int mat_size, dtype *outMat)
{
	cl_command_queue queue_device;
	cl_int err;
	dtype ret = 0.0;

	//********���ò���**********/
	int j = 0;
	err = clSetKernelArg(kern_trigMat_mul, 0, sizeof(cl_mem), &matBuf);
	err |= clSetKernelArg(kern_trigMat_mul, 1, sizeof(cl_mem), &auxBuf);
	err |= clSetKernelArg(kern_trigMat_mul, 2, sizeof(int), &mat_size);

	//ִ���ں�,
	size_t global_size[2] = { mat_size,mat_size };
	//size_t local_size[2] = { 3,3 };
	err = clEnqueueNDRangeKernel(queue, kern_trigMat_mul,
		2, NULL,	//1D work space
		global_size,
		NULL,		//local_size,
		0, NULL, NULL);
	checkErr(err, __FILE__, __LINE__);
	clFinish(queue);

	if (outMat != NULL)
	{
		err = clEnqueueReadBuffer(queue, matBuf, CL_TRUE, 0,
			sizeof(dtype)*mat_size*mat_size, outMat, 0, NULL, NULL);
		checkErr(err, __FILE__, __LINE__);
	}

#if DEBUG_TRIGMAT_INV==1
	printBuf2D(stdout, queue, matBuf, mat_size, mat_size, "Inverse:");

#endif
}