

/*
����g= coeff*J*ex
*/
__kernel void kern_compute_g(
	//const int cnp,
	//const int pnp,
	const int nCams,
	const int n3Dpts,
	const int n2Dprojs,
	const dtype coeff,
	__global dtype *JA,
	__global dtype *JB,
	__global int *jac_idx,
	__global dtype *ex,
	__global dtype *g)
{
	int tr = get_global_id(0);		//g�����еĵ�tr��Ԫ��
	int idx = 0;
	int i, j, k;
	int addr1, addr2;
	dtype sum = 0;

	if (tr < cnp*nCams)		//����ga
	{
		j = (int)(tr / cnp);	//�õ��ù������Ӧ���j
		k = tr - j*cnp;		//�õ��ù������Ӧ���j�ĵ�k������
		for (i = 0; i < n3Dpts; i++)
		{
			idx = jac_idx[i*nCams + j];	//�õ�JA_ij��e_ij�ĵ�����
			addr1 = idx * 2 * cnp + k;
			addr2 = idx * 2;
			//if (idx >= 0)
			//	sum = sum + JA[idx * 2 * cnp + k] * ex[idx * 2]
			//	+ JA[idx * 2 * cnp + cnp + k] * ex[idx * 2 + 1];
			if (idx >= 0)
				sum = sum + JA[addr1] * ex[addr2++]
				+ JA[addr1 + cnp] * ex[addr2];
		}
	}
	else {		//����gb
		i = ((int)(tr - nCams*cnp) / pnp);	//�õ��ù������Ӧ3D��i
		k = tr - nCams*cnp - i*pnp;		//�õ��ù������Ӧ3D��i�еĵ�k���������
		for (j = 0; j < nCams; j++)
		{
			idx = jac_idx[i*nCams + j];	//�õ�JB_ij��e_ij�ĵ�����
			addr1 = idx * 2 * pnp + k; 
			addr2 = idx * 2;
			//if (idx >= 0)
			//	sum = sum + JB[idx * 2 * pnp + k] * ex[idx * 2]
			//	+ JB[idx * 2 * pnp + pnp + k] * ex[idx * 2 + 1];

			if (idx >= 0)
				sum = sum + JB[addr1] * ex[addr2++]
				+ JB[addr1+ pnp] * ex[addr2];
		}
	}
	g[tr] = coeff*sum;
}