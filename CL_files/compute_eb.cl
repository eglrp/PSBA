

/*
����eb��eb=gb-(W^t)*dpa
*/
__kernel void kern_compute_eb(
	//const int cnp,
	//const int pnp,
	const int nCams,
	const int n3Dpts,
	const int n2Dprojs,
	__global int *blk_idx,
	__global dtype *Wblks,
	__global dtype *dp,		//ʹ��dpa,����Ҫƫ��
	__global dtype *g,		//ʹ��gb,��Ҫga��ƫ����
	__global dtype *eab)
{
	int tr = get_global_id(0);		//��r������
	int i = (int)(tr / pnp);		//��i��3D��
	int c = tr - i*pnp;				//�õ�W_i*��c�С���pnp��Ԫ��

	dtype sum = 0;

	//����Y_0j*gb0,Y_1j*gb1,...,Y_nj*gbn
	int Wbase = 0;
	int dpBase = 0;
	int gbBase = nCams*cnp;
	for (int j = 0; j < nCams; j++) {		//W�Ƿֿ����
		int idx = blk_idx[i*nCams + j];		//�ҵ���Ӧ��Wij��
		if (idx >= 0) {
			Wbase = idx*cnp*pnp;
			for (int k = 0; k < cnp; k++) {		//W_ij��c����dpa_j�ĵ��
				sum = sum + Wblks[Wbase + k*pnp + c] * dp[dpBase + k];
			}
		}
		dpBase = dpBase + cnp;
	}

	eab[gbBase + tr] = g[gbBase + tr] - sum;
	//gb[tr] = i;
}