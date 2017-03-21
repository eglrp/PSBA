

/*
����Yij�����, Y_ij=W_ij*(Vi^-1),
*/
__kernel void kern_compute_Yblks(
	//const int cnp,
	//const int pnp,
	const int nCams,
	const int n3Dpts,
	__global int *iidx,
	__global dtype *Wblks,
	__global dtype *Vinv,	//ֻ�����������
	__global dtype *Yblks)
{
	int r = get_global_id(0);		//��ǰ��������Y_ij�е�row
	int tc = get_global_id(1);
	int idx = (int)(tc / pnp);		//
	int c = tc - idx*pnp;			//��ǰ��������Y_ij�е�col

									//Yij=Wij*(Vi^-1), �Ӷ�Yij�е�(r,c)λ�þ���Wij�е�r�е��Vi^-1�е�c��
	int YWbase = idx*cnp*pnp + r*pnp;
	//�õ�Vi^-1�Ļ�����
	int VinvBase = iidx[idx] * pnp*pnp;

	if (c>0) {		//Yij�ĺ����Ԫ��
		Yblks[YWbase + c] = Wblks[YWbase] * Vinv[VinvBase + c*pnp]		//c�еĵ�һ��Ԫ��(0,c)ʹ�öԽǵ�Ԫ��(c,0)��
			+ Wblks[YWbase + 1] * Vinv[VinvBase + c*pnp + 1]				//c�еĵ�һ��Ԫ��(1,c)ʹ�öԽǵ�Ԫ��(c,1)��
			+ Wblks[YWbase + 2] * Vinv[VinvBase + 2 * pnp + c];				//���һ��ʹ��ԭʼ��Ԫ��

																			//Yblks[YWbase + c] = Vinv[VinvBase + c*pnp]+ Vinv[VinvBase + c*pnp + 1]+	 Vinv[VinvBase + 2 * pnp + c];	
	}
	else {
		Yblks[YWbase + c] = Wblks[YWbase] * Vinv[VinvBase]
			+ Wblks[YWbase + 1] * Vinv[VinvBase + 1 * pnp]
			+ Wblks[YWbase + 2] * Vinv[VinvBase + 2 * pnp];
	}

}