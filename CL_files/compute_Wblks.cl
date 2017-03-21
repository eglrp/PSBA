

/*
�������Wblks, W11,W21,..,Wn1,..,Wnm
ÿ�����������W�е�һ��Ԫ��
*/
__kernel void kern_compute_Wblks(
	//const int cnp,
	//const int pnp,
	const int nCams,
	const int n3Dpts,
	const int n2Dprojs,
	__global dtype *JA,
	__global dtype *JB,
	__global int *iidx,
	__global int *jidx,
	__global dtype *Wblks,
	const dtype coeff)
{
	int r = get_global_id(0);		//��W_ij�е�row
	int tc = get_global_id(1);
	int idx = (int)(tc / pnp);		//
	int c = tc - idx*pnp;			//��W_ij�е�col

	dtype sum = 0;

	//����Wij��r,c��ȡAij��r����Bij��c���������Aij��Bij������
	sum = JA[idx * 2 * cnp + r] * JB[idx * 2 * pnp + c]
		+ JA[idx * 2 * cnp + cnp + r] * JB[idx * 2 * pnp + pnp + c];

	//��ŵ�W��
	sum = coeff*sum;
	Wblks[idx*cnp*pnp + r*pnp + c] = sum;
}