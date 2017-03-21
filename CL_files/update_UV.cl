
/*
ʹ��mu����UV�ĶԽ���Ԫ��
*/
__kernel void kern_update_UV(
	//const int cnp,
	//const int pnp,
	const int nCams,
	const int n3Dpts,
	__global dtype *U,
	__global dtype *V,
	const dtype mu)
{
	int j, rc;
	int idx = get_global_id(0);

	if (idx < nCams*cnp)
	{
		j = (int)(idx / cnp);	//�õ���Ӧ���j
		rc = idx - j*cnp;		//�õ����j�ϵĶԽ�Ԫ��
		rc = j*cnp*cnp + rc*cnp + rc;
		U[rc] = U[rc] + mu;
	}
	else {
		idx = idx - nCams*cnp;
		j = (int)(idx / pnp);	//�õ���Ӧ3D��j
		rc = idx - j*pnp;
		rc = j*pnp*pnp + rc*pnp + rc;
		V[rc] = V[rc] + mu;
	}
}