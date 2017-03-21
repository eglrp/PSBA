
/*
����dpb, dpb=(V^-1)*gb, Vinvֻ�洢��������

*/
__kernel void kern_compute_dpb(
	//const int cnp,
	//const int pnp,
	const int nCams,
	const int n3Dpts,
	__global dtype *Vinv,
	__global dtype *eab,		//ʹ��eb
	__global dtype *dp)		//ʹ��dpb
{
	int tr = get_global_id(0);		//��tr������
	int i = (int)(tr / pnp);		//��i��3D��
	int r = tr - i*pnp;				//�õ�Vinv_ii��r��

	dtype sum = 0;

	int Vinv_base = i*pnp*pnp;
	int gbase = nCams*cnp + i*pnp;
	if (r < 2) {	//ǰ����
		sum = Vinv[Vinv_base + r*pnp] * eab[gbase]					//��һ��Ԫ��(r,0)����
			+ Vinv[Vinv_base + pnp + r] * eab[gbase + 1]		//�ڶ���Ԫ��(r,1)ʹ�öԽ�Ԫ��(1,r)
			+ Vinv[Vinv_base + 2 * pnp + r] * eab[gbase + 2];	//������Ԫ��(r,2)ʹ�öԽ�Ԫ��(2,r)
	}
	else {
		sum = Vinv[Vinv_base + r*pnp] * eab[gbase]
			+ Vinv[Vinv_base + r*pnp + 1] * eab[gbase + 1]
			+ Vinv[Vinv_base + r*pnp + 2] * eab[gbase + 2];
	}

	dp[nCams*cnp + tr] = sum;
}