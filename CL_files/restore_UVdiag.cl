
__kernel void kern_restore_UVdiag(
	const int nCams,
	const int n3Dpts,
	__global dtype *U,
	__global dtype *V,
	__global dtype *UVdiag)
{
	int j, rc;
	int idx = get_global_id(0);

	if (idx < nCams*cnp)
	{
		j = (int)(idx / cnp);	//�õ���Ӧ���j
		rc = idx - j*cnp;		//�õ����j�ϵĶԽ�Ԫ��
		rc = j*cnp*cnp + rc*cnp + rc;
		U[rc] = UVdiag[idx];
	}
	else {
		int idx2 = idx - nCams*cnp;
		j = (int)(idx2 / pnp);	//�õ���Ӧ3D��j
		rc = idx2 - j*pnp;
		rc = j*pnp*pnp + rc*pnp + rc;
		V[rc] = UVdiag[idx];
	}
}