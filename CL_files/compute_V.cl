

/*
����V����
*/
__kernel void kern_compute_V(
	//const int cnp,
	//const int pnp,
	const int nCams,
	const int n3Dpts,
	const int n2Dprojs,
	__global dtype *JB,
	__global int *jac_idx,
	__global dtype *V,
	__global dtype *UVdiag,
	const dtype coeff)
{
	int r = get_global_id(0);
	int tc = get_global_id(1);	//
								//����ó������ĸ�Vi,�Լ�Ԫ����Vi�е���c
	int i = (int)(tc / pnp);	//Vi
	int c = tc - i*pnp;

	dtype sum = 0;
	for (int j = 0; j < nCams; j++)
	{
		//����(Bij)^T*Bij�ĵ�r,c��Ԫ��, ��Bij�ĵ�r�����c�еĵ��
		int idx = jac_idx[i*nCams + j];	//���ҵ���Ӧ��Bij����JB�е�����λ��
		if (idx >= 0)
			sum = sum + JB[idx * 2 * pnp + r] * JB[idx * 2 * pnp + c]
			+ JB[idx * 2 * pnp + pnp + r] * JB[idx * 2 * pnp + pnp + c];
	}
	//��sumֵ����V(r,tc)
	sum = coeff*sum;
	V[i*pnp*pnp + r*pnp + c] = sum;
	if (r == c)
		UVdiag[nCams*cnp + i*pnp + r] = sum;
}
