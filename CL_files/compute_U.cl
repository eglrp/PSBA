/*
����U����
��������U�У�ͬʱ��Խ��ߴ����UVdiag��
*/
__kernel void kern_compute_U(
	//const int cnp,
	const int nCams,
	const int n3Dpts,
	const int n2Dprojs,
	__global dtype *JA,
	__global int *jac_idx,
	__global dtype *U,
	__global dtype *UVdiag,
	const dtype coeff)
{
	int r = get_global_id(0);
	int tc = get_global_id(1);		//��U�ϳɾ����е�col
	int j = (int)(tc / cnp);			//Uj
	int c = tc - j*cnp;					//Uj�е�col

	dtype sum = 0;
	for (int i = 0; i < n3Dpts; i++)
	{
		//����(Aij)^T*Aij�ĵ�r,c��Ԫ��, ��Aij�ĵ�r�����c�еĵ��
		int idx = jac_idx[i*nCams + j];	//���ҵ���Ӧ��Aij����JA�е�����λ��
		if (idx >= 0)
			sum = sum + JA[idx * 2 * cnp + r] * JA[idx * 2 * cnp + c]
			+ JA[idx * 2 * cnp + cnp + r] * JA[idx * 2 * cnp + cnp + c];
	}
	//��sumֵ����Uj(r,c)
	sum = coeff*sum;
	U[j*cnp*cnp + r*cnp + c] = sum;
	if (r == c)
		UVdiag[j*cnp + r] = sum;
}