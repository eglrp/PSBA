
/*
require: OpenCL>2.0
*/


/*****************************************************************************************/
__kernel void kern_cholesky_s2(
	__global dtype* mat,
	__global dtype* diagInv,
	__global dtype* ret,
	const int mat_size,
	const int j);

/*
cholesky�ֽ�
step 1: ����Tij=Aij-sum(Lik*Ljk^t)�е�uvԪ�أ��Լ�����Խǿ�
jΪ�����ĵ�ǰ��
*/
__kernel void kern_cholesky(
	__global dtype *mat,		//ԭʼ���� �� �������
	__global dtype *diagInv,	//�����Lii^-1, �ɹ����Ǿ�������ʹ��
	__global dtype *ret,
	__local dtype *T,
	__local dtype *L,
	const int mat_size,
	const int j)
{
	dtype3 *ptr1, *ptr2;
	dtype sum, t1, t2, t3, t4, t5;
	int u = get_local_id(0);
	int v = get_local_id(1);
	int i = j + get_group_id(0);

	if (get_global_id(0) == 0 && get_global_id(1) == 0)
	{
		*ret = 0.0;
	}

	//����Tij = Aij - sum(Lik*Ljk^t)�е�uvԪ��
	int ijuv_addr = (i * 3 + u)*mat_size + j * 3 + v;
	sum = mat[ijuv_addr];	//Aij_uvԪ��
	for (int k = 0; k < j; k++)
	{
		int Lik_base = (i * 3 + u)*mat_size + k * 3;		//Lik�ĵ�u��
		int Ljk_base = (j * 3 + v)*mat_size + k * 3;		//Ljk�ĵ�v��
		//sum -= mat[Lik_base++] * mat[Ljk_base++];
		//sum -= mat[Lik_base++] * mat[Ljk_base++];
		//sum -= mat[Lik_base] * mat[Ljk_base];
		ptr1 = (dtype3 *)&mat[Lik_base];
		ptr2 = (dtype3 *)&mat[Ljk_base];
		sum -= dot(*ptr1, *ptr2);
	}
	mat[ijuv_addr] = sum;		//����Tij_uv��ԭ����
	T[u * 3 + v] = sum;		//ͬʱ���浽���ػ��棬�Խ��߿�ʹ��

	barrier(CLK_GLOBAL_MEM_FENCE);		//������ͬ��

										//����Խǿ�,����Lii
	if (i == j) {
		switch (u * 3 + v)
		{
		case 0:		//L00 or L(0)
			L[0] = sqrt(T[0]);
			mat[ijuv_addr] = L[0];
			if (!isfinite(L[0])) *ret = 1.0;
			break;
		case 1:		//L01,L02
		case 2:
			mat[ijuv_addr] = 0;
			break;
		case 3:		//L10
			L[1] = T[1 * 3 + 0] / sqrt(T[0]);
			mat[ijuv_addr] = L[1];
			if (!isfinite(L[1])) *ret = 1.0;
			break;
		case 4:		//L11
			L[2] = sqrt(T[1 * 3 + 1] - T[1 * 3 + 0] * T[1 * 3 + 0] / T[0]);;
			mat[ijuv_addr] = L[2];
			if (!isfinite(L[2])) *ret = 1.0;
			break;
		case 5:		//L12
			mat[ijuv_addr] = 0;
			break;
		case 6:		//L20 or L(3)
			L[3] = T[2 * 3 + 0] / sqrt(T[0]);
			mat[ijuv_addr] = L[3];
			if (!isfinite(L[3])) *ret = 1.0;
			break;
		case 7:		//L21 or L(4)
			L[4] = sqrt(T[0] / (T[0] * T[1 * 3 + 1] - T[1 * 3 + 0] * T[1 * 3 + 0]))*(T[2 * 3 + 1] - T[1 * 3 + 0] * T[2 * 3 + 0] / T[0]);
			mat[ijuv_addr] = L[4];
			if (!isfinite(L[4])) *ret = 1.0;
			break;
		case 8:		//L22 or (5)
					//t1 = T[0] / (T[0] * T[1 * 3 + 1] - T[1 * 3 + 0] * T[1 * 3 + 0]);
					//t2 = T[2 * 3 + 1] - T[1 * 3 + 0] * T[2 * 3 + 0] / T[0];
					//t2 = t2*t2;
					//L[5] = sqrt(T[2 * 3 + 2] - T[2 * 3 + 0] * T[2 * 3 + 0] / T[0] - t1*t2);
			t1 = -T[2 * 3 + 2] * T[1 * 3 + 0] * T[1 * 3 + 0];
			t2 = 2 * T[2 * 3 + 1] * T[1 * 3 + 0] * T[2 * 3 + 0];
			t3 = -T[1 * 3 + 1] * T[2 * 3 + 0] * T[2 * 3 + 0];
			t4 = T[0 * 3 + 0] * (T[1 * 3 + 1] * T[2 * 3 + 2] - T[2 * 3 + 1] * T[2 * 3 + 1]);
			t5 = -T[1 * 3 + 0] * T[1 * 3 + 0] + T[0 * 3 + 0] * T[1 * 3 + 1];
			L[5] = sqrt((t1 + t2 + t3 + t4) / t5);
			mat[ijuv_addr] = L[5];
			if (!isfinite(L[5])) *ret = 1.0;
			break;
		default:
			break;
		}
	}
	//����Lii�������
	barrier(CLK_GLOBAL_MEM_FENCE);		//������ͬ��
	if (i == j) {
		int diag_addr = j * 3 * 3 + u * 3 + v;
		switch (u * 3 + v)
		{
		case 0:		//L00 or L(0)
			t1 = 1 / L[0];
			diagInv[diag_addr] = t1;
			if (!isfinite(t1)) *ret = 1.0;
			break;
		case 1:		//L01,L02
		case 2:
			diagInv[diag_addr] = 0;
			break;
		case 3:		//L10 or L(1)
			t1 = -L[1] / (L[0] * L[2]);
			diagInv[diag_addr] = t1;
			if (!isfinite(t1)) *ret = 1.0;
			break;
		case 4:		//L11 or L(2)
			t1 = 1 / L[2];
			diagInv[diag_addr] = t1;
			if (!isfinite(t1)) *ret = 1.0;
			break;
		case 5:		//L12
			diagInv[diag_addr] = 0;
			break;
		case 6:		//L20 or L(3)
			t1 = (L[1] * L[4] - L[2] * L[3]) / (L[0] * L[2] * L[5]);
			diagInv[diag_addr] = t1;
			if (!isfinite(t1)) *ret = 1.0;
			break;
		case 7:		//L21 or L(4)
			t1 = -L[4] / (L[2] * L[5]);
			diagInv[diag_addr] = t1;
			if (!isfinite(t1)) *ret = 1.0;
			break;
		case 8:		//L22 or L(5)
			t1 = 1 / L[5];
			diagInv[diag_addr] = t1;
			if (!isfinite(t1)) *ret = 1.0;
			break;
		default:
			break;
		}
	}
	barrier(CLK_GLOBAL_MEM_FENCE);		//������ͬ��

	/************************************************************************/
	/************************************************************************/
	//����kern_cholesky_m5_s2
	if (get_global_id(0) == 0 && get_global_id(1) == 0 && get_global_size(0) >3 && *ret == 0.0)
	{
		//urow_base = mat_size - (j + 1) * 3;		//ʣ����������
		void(^s2_wrapper)(void) = ^{
			kern_cholesky_s2(mat, diagInv, ret, mat_size, j);
		};
		size_t    global_size[2] = { get_global_size(0) - 3, 3 };
		size_t    local_size[2] = { 3,3 };
		ndrange_t ndrange = ndrange_2D(global_size, local_size);
		enqueue_kernel(
			get_default_queue(),
			CLK_ENQUEUE_FLAGS_WAIT_KERNEL,
			ndrange, s2_wrapper
		);
	}

}


/*
cholesky�ֽ�
step 2 ����ʣ��Ԫ��, Lij=Tij*(Ljj^-t)
*/
__kernel void kern_cholesky_s2(
	__global dtype* mat,
	__global dtype* diagInv,
	__global dtype* ret,
	const int mat_size,
	const int j)
{
	dtype3 *ptr1, *ptr2;
	dtype sum = 0.0;
	int u = get_local_id(0);
	int v = get_local_id(1);
	int i = j + get_group_id(0) + 1;

	//����Lij=Tij*(Ljj^-t)�е�uvԪ��
	int ijuv_addr = (i * 3 + u)*mat_size + j * 3 + v;
	int jivu_addr = (j * 3 + v)*mat_size + i * 3 + u;
	int urow_base = (i * 3 + u)*mat_size + j * 3;	//Tij�ĵ�u��
	int vrow_base = j * 3 * 3 + v * 3;				//(Ljj^-1)�ĵ�v��

	ptr1 = (dtype3 *)&mat[urow_base];
	ptr2 = (dtype3 *)&diagInv[vrow_base];
	sum += dot(*ptr1, *ptr2);
	//sum += mat[urow_base++] * diagInv[vrow_base++];
	//sum += mat[urow_base++] * diagInv[vrow_base++];
	//sum += mat[urow_base] * diagInv[vrow_base];

	barrier(CLK_LOCAL_MEM_FENCE);		//������ͬ��

	mat[ijuv_addr] = sum;
	mat[jivu_addr] = 0;

	/***********************************************************/
	//����kern_cholesky_m5_s1
	if (get_global_id(0) == 0 && get_global_id(1) == 0 && get_global_size(0) >= 3 && *ret == 0.0)
	{
		void(^s1_wrapper)(local void *, local void *) =
			^ (local void *TT, local void *LL) {
			kern_cholesky(mat, diagInv, ret, TT, LL, mat_size, j + 1);
		};
		size_t    global_size[2] = { get_global_size(0), 3 };
		size_t    local_size[2] = { 3,3 };
		ndrange_t ndrange = ndrange_2D(global_size, local_size);
		enqueue_kernel(
			get_default_queue(),
			CLK_ENQUEUE_FLAGS_WAIT_KERNEL,
			ndrange, s1_wrapper,
			(unsigned int)(3 * 3 * sizeof(dtype)),		//size of local memory TT
			(unsigned int)(6 * sizeof(dtype))			//size of local memory LL
		);
	}

}

/*****************************************************************************************/

/*****************************************************************************************/
/*
���������ǿ�������, �浽in��������
��kernelֻ�����i��б�߿飬Ȼ����ü�����һ��б�߿�i+1��kernel_trigMatInv
*/
__kernel void kern_trigMat_inv(
	__global dtype *in,
	__global dtype *diagBlk,	//����cholesky�����Lii^-1
	__global dtype *ret,
	const int mat_size,		//ԭʼ����ĳߴ�
	const int ii			//��ii��б�߿�
)
{
	int addr, addr2, ijuv_addr, jivu_addr;

	int tr = get_global_id(0);
	int j = (int)(tr / 3);		//��ǰ�������к�
	int i = ii + j;
	int u = tr - j * 3;			//��ǰ�����ĵ�u��
	int v = get_global_id(1);	//��ǰ�����ĵ�v��

	dtype tt;
	dtype L0, L1, L2, L3, L4, L5;

	ijuv_addr = (i * 3 + u)*mat_size + j * 3 + v;
	jivu_addr = (j * 3 + v)*mat_size + i * 3 + u;

	//����ǶԽ��߿飬��diagBlkȡ����Lii^-1���浽out�ĶԽ�����
	if (i == j)
	{
		in[ijuv_addr] = diagBlk[i * 9 + v * 3 + u];
		barrier(CLK_GLOBAL_MEM_FENCE);	//ͬ������ʹ��mat��ȡ���
	}
	else
	{
		//���ڷǶԽǿ�
		//1������T_ij=sum(L_ik*X_kj), k=j to i-1,  ע��X_kj^t����out��������
		tt = 0.0;
		for (int k = j; k < i; k++)
		{
			//L_ik��u���� X_kj^t��v�еĵ��
			addr = (i * 3 + u)*mat_size + k * 3;
			addr2 = (j * 3 + v)*mat_size + k * 3;
			tt += in[addr++] * in[addr2++];
			tt += in[addr++] * in[addr2++];
			tt += in[addr] * in[addr2];
		}
		in[jivu_addr] = tt;		//T_ij^t��u,vԪ�ش浽in��������

								//ͬ��
		barrier(CLK_GLOBAL_MEM_FENCE);

		//����X_ij=(L_ii^-1)*T_ij, ���洢����L_ii^-t ��T_ij^t, ��ôӦ��ȡL_ii^-t��u����T_ij^t��v�еĵ��
		tt = 0.0;
		addr = i * 3 * mat_size + i * 3 + u;		//u�е�һ��Ԫ��
		addr2 = (j * 3 + v)*mat_size + i * 3;	//v�е�һ��Ԫ��
		tt += in[addr] * in[addr2++];
		tt += in[addr + mat_size] * in[addr2++];
		tt += in[addr + 2 * mat_size] * in[addr2];

		//ͬ��
		barrier(CLK_GLOBAL_MEM_FENCE);
		if (i != j) {
			in[jivu_addr] = -tt;
			//out[ijuv_addr] = 0;
		}
	}

	//����ii+1��б�߿�ļ����kernel
	addr = mat_size - (ii + 1) * 3;		//ʣ����������
	if (tr == 0 && v == 0 && addr>0)
	{
		void(^kernel_trigMatInv_wrapper)(void) =
			^{
			kern_trigMat_inv(in,diagBlk, ret, mat_size, ii + 1);
		};
		size_t    global_size[2] = { addr, 3 };
		size_t    local_size[2] = { 3,3 };
		ndrange_t ndrange = ndrange_2D(global_size, local_size);
		enqueue_kernel(
			get_default_queue(),
			CLK_ENQUEUE_FLAGS_WAIT_KERNEL,
			ndrange, kernel_trigMatInv_wrapper
		);
	}
}

/*****************************************************************************************/

/*****************************************************************************************/

__kernel void kern_fill_rest(
	__global dtype *in,
	__global dtype *diag,
	const int mat_size);

/*
����S^-1=(L^-t)*(L^-1)��
inΪ�����ǿ����L^-t
*/
__kernel void kern_trigMat_mul(
	__global dtype *in,
	__global dtype *diag,
	const int mat_size)
{
	dtype val;
	int u = get_global_id(0);		//����������Ԫ�ص��к�
	int v = get_global_id(1);		//����������Ԫ�ص��к�

	if (v > u) return;

	//(i,j)��ֵΪin�����i����j�еĳ˻���
	int u_addr, v_addr;
	u_addr = u*mat_size + u;		//��i�п�ʼ��֮ǰ����˶�Ϊ0
	v_addr = v*mat_size + u;
	val = 0;
	for (int k = u; k < mat_size; k++)
	{
		val += in[u_addr++] * in[v_addr++];
	}
	if (u == v)
		diag[u] = val;
	else
		in[u*mat_size + v] = val;

	//call kernel to fill diagonal element
	if (u == 0 && v == 0)
	{
		void(^kernel_fill_rest_wrapper)(void) =
			^{
			kern_fill_rest(in,diag, mat_size);
		};
		size_t    global_size[2] = { mat_size, mat_size };
		//size_t    local_size[2] = { 3,3 };
		ndrange_t ndrange = ndrange_2D(global_size);
		enqueue_kernel(
			get_default_queue(),
			CLK_ENQUEUE_FLAGS_WAIT_KERNEL,
			ndrange, kernel_fill_rest_wrapper
		);

	}
}

/*
1,д��Խ�Ԫ��
2,��������Ԫ�ظ��Ƶ�������
*/
__kernel void kern_fill_rest(
	__global dtype *in,
	__global dtype *diag,
	const int mat_size)
{
	int addr1, addr2;
	int u = get_global_id(0);		//����������Ԫ�ص��к�
	int v = get_global_id(1);		//����������Ԫ�ص��к�

	if (v > u) return;

	addr1 = u*mat_size + v;
	if (u == v)
	{
		in[addr1] = diag[u];
	}
	else {
		addr2 = v*mat_size + u;
		in[addr2] = in[addr1];
	}
}