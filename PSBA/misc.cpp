#include "stdafx.h"

#include <math.h>
#include <stdlib.h>
#include "psba.h"

/* unit quaternion from vector part */
#define _MK_QUAT_FRM_VEC(q, v){                                     \
  (q)[1]=(v)[0]; (q)[2]=(v)[1]; (q)[3]=(v)[2];                      \
  (q)[0]=sqrt(1.0 - (q)[1]*(q)[1] - (q)[2]*(q)[2]- (q)[3]*(q)[3]);  \
}

/* 
���룺
	inp		-->�����ڲ�K(5,��ѡ)������(5,��ѡ),��ת��4����Ԫ������λ�ƣ�3��
	nin		-->���������ά��
�����
	outp	-->�����ڲ�K(5,��ѡ),����(5,��ѡ),��ת��3����Ԫ�����������֣�,λ�ƣ�3��
	nout	-->���������ά��
*/
void quat2vec(dtype *inp, int nin, dtype *outp, int nout)
{
	dtype mag, sg;
	register int i;

	/* intrinsics & distortion */
	if (nin>7) // are they present?
		for (i = 0; i<nin - 7; ++i)
			outp[i] = inp[i];
	else
		i = 0;

	/* rotation */
	/* normalize and ensure that the quaternion's scalar component is non-negative;
	* if not, negate the quaternion since two quaternions q and -q represent the
	* same rotation
	*/
	mag = sqrt(inp[i] * inp[i] + inp[i + 1] * inp[i + 1] + inp[i + 2] * inp[i + 2] + inp[i + 3] * inp[i + 3]);
	sg = (inp[i] >= 0.0) ? 1.0 : -1.0;
	mag = sg / mag;
	outp[i] = inp[i + 1] * mag;
	outp[i + 1] = inp[i + 2] * mag;
	outp[i + 2] = inp[i + 3] * mag;
	i += 3;

	/* translation*/
	for (; i<nout; ++i)
		outp[i] = inp[i + 1];
}


/* 
���룺
inp		-->�����ڲ�K(5,��ѡ),����(5,��ѡ),��ת��3����Ԫ�����������֣�,λ�ƣ�3��
nin		-->���������ά��
�����
outp	-->�����ڲ�K(5,��ѡ)������(5,��ѡ),��ת��4����Ԫ������λ�ƣ�3��
nout	-->���������ά��
*/
void vec2quat(dtype *inp, int nin, dtype *outp, int nout)
{
	dtype *v, q[4];
	register int i;

	/* intrinsics & distortion */
	if (nin>7 - 1) // are they present?
		for (i = 0; i<nin - (7 - 1); ++i)
			outp[i] = inp[i];
	else
		i = 0;

	/* rotation */
	/* recover the quaternion from the vector */
	v = inp + i;
	_MK_QUAT_FRM_VEC(q, v);
	outp[i] = q[0];
	outp[i + 1] = q[1];
	outp[i + 2] = q[2];
	outp[i + 3] = q[3];
	i += 4;

	/* translation */
	for (; i<nout; ++i)
		outp[i] = inp[i - 1];
}


/*
* ����Ԫ��ת��Ϊ��ת����
* dim=3 ==><v>
* dim=4 ==><s,v>
*/
void quat2matrix(dtype *in,int dim, dtype *out)
{
	dtype s, vx, vy, vz;
	if (dim == 3) {
		vx = in[0]; vy = in[1]; vz = in[2];
		s = sqrt(1 - vx * vx - vy * vy - vz * vz);
	}
	else {
		s = in[0];
		vx = in[1]; vy = in[2]; vz = in[3];
	}
	out[0] = 1 - 2 * vy*vy - 2 * vz*vz;		//R11
	out[1] = 2 * vx*vy - 2 * vz*s;			//R12
	out[2] = 2 * vx*vz + 2 * vy*s;			//R13
	out[3] = 2 * vx*vy + 2 * vz*s;			//R21
	out[4] = 1 - 2 * vx*vx - 2 * vz*vz;		//R22
	out[5] = 2 * vy*vz - 2 * vx*s;			//R23
	out[6] = 2 * vx*vz - 2 * vy*s;			//R31
	out[7] = 2 * vy*vz + 2 * vx*s;			//R32
	out[8] = 1 - 2 * vx*vx - 2 * vy*vy;		//R33
}

/*
����Ԫ����ʾ�Ļ������ת��Ϊ��ת�����ʾ��������������뵽out��
in������һ�������dim+ndummy��doubleֵ
*/
void camsFormatTrans(dtype *in, int nin, int dim,int stubForTrans, dtype *out)
{
	dtype* inPtr;
	dtype* outPtr;
	for (int i = 0; i < nin; i++)
	{
		inPtr = in + i*(dim + stubForTrans);
		outPtr=out + (9 + stubForTrans) * i;
		quat2matrix(inPtr, dim, outPtr);
		outPtr[9] = inPtr[dim];
		outPtr[10] = inPtr[dim + 1];
		outPtr[11] = inPtr[dim + 2];
	}
}

/* auxiliary memory allocation routine with error checking */
void *emalloc_(char *file, int line, size_t sz)
{
	void *ptr;

	ptr = (void *)malloc(sz);
	if (ptr == NULL) {
		fprintf(stderr, "SBA: memory allocation request for %u bytes failed in file %s, line %d, exiting", sz, file, line);
		exit(1);
	}

	return ptr;
}

/*
����������L2����
*/
dtype compute_L2_sq(int n, dtype *x)
{
	dtype sum = 0;
	for (int i = 0; i < n; i++)
		sum += x[i] * x[i];
	return sum;
}

/*
�����ĵ��
*/
dtype dotProduct(dtype *a, dtype *b, int size)
{
	dtype sum = 0.0;

	for (int i = 0; i < size; i++)
		sum += a[i] * b[i];
	return sum;
}


/*
camsPtsIdxΪ���������ɵ�2D���������飬camsPtsIdxPtrָʾÿһ���������3D�㣨2Dͼ��㣩������������
iidx����ָʾ2D��������һ��3D��
jidx����ָʾ2D��������һ�����
blk_idx����ָʾ3D��i�����j�Ƕ�Ӧ����һ��ͼ���
*/
void generate_idxs(int nCams, int n3Dpts, int n2Dprojs, dtype *impts_data, char *vmask,
	int *comm3DIdx, int *comm3DIdxCnt, int *iidx, int *jidx, int *blk_idx)
{
	int tmp;
	int idx;
	int *idx_arr, *cnt_arr;
	int rowsize = nCams*n3Dpts;

	memset(comm3DIdxCnt, 0, nCams*nCams * sizeof(int));

	int impts_idx = 0;
	for (int i = 0; i <n3Dpts; i++)
	{
		for (int j = 0; j < nCams; j++)
		{
			if (*(vmask + i*nCams + j)) {

				iidx[impts_idx] = i;
				jidx[impts_idx] = j;
				blk_idx[i*nCams + j] = impts_idx;

				//*****Ѱ��j��k����Ƿ��й�ͬ��, �����������ǲ���
				for (int k = 0; k <= j; k++)
				{
					if (*(vmask + i*nCams + k))		//���������ӵ���Ӧ��[j,k]���б���
					{
						tmp = comm3DIdxCnt[j*nCams + k]++;
						comm3DIdx[j*rowsize + k*n3Dpts + tmp] = i;
						if (k != j) {
							comm3DIdxCnt[k*nCams + j]++;
							comm3DIdx[k*rowsize + j*n3Dpts + tmp] = i;
						}
					}
				}
				//*****
				impts_idx++;
			}
			else blk_idx[i*nCams + j] = -1;
		}
	}
}
