

/*
����ea��ֵ��ea=ga-Y*gb
*/
__kernel void kern_compute_ea(
	//const int cnp,
	//const int pnp,
	const int nCams,
	const int n3Dpts,
	const int n2Dprojs,
	__global int *blk_idx,
	__global dtype *Yblks,
	__global dtype *g,
	__global dtype *ea)
{
	int tr = get_global_id(0);		//��r������
	int j = (int)(tr / cnp);		//��j�����
	int r = tr - j*cnp;				//�õ�Y_*j��r��

	dtype sum = 0;

	//����Y_0j*gb0,Y_1j*gb1,...,Y_nj*gbn
	int Ybase = 0;
	int gbase = nCams*cnp;
	for (int i = 0; i < n3Dpts; i++) {		//Y�Ƿֿ����
		int idx = blk_idx[i*nCams + j];		//�ҵ���Ӧ��Yij��
		if (idx >= 0) {
			Ybase = idx*cnp*pnp + r*pnp;
			sum = sum + Yblks[Ybase] * g[gbase]
				+ Yblks[Ybase + 1] * g[gbase + 1]
				+ Yblks[Ybase + 2] * g[gbase + 2];
		}
		gbase = gbase + pnp;
	}
	ea[tr] = g[tr] - sum;
}