

/*
����S���� U-YW^t
*/
__kernel void kern_compute_S(
	//const int cnpp,
	//const int pnp,
	const int nCams,
	const int n3Dpts,
	__global int *blk_idx,
	__global dtype *U,
	__global dtype *Yblks,
	__global dtype *Wblks,
	__global int *comm3DIdx,
	__global int *comm3DIdxCnt,
	__global dtype *S)
{
	int idx;
	int Yidx, Widx, Ybase, Wbase;
	int tr = get_global_id(0);		//
	int tc = get_global_id(1);		//
	//int cnp = cnpp;
	//��r,c�䵽��Ӧ��Ui
	int k = (int)(tr / cnp);		//S��S_kl������k��
	int l = (int)(tc / cnp);		//S��S_kl������l��
	int r = tr - k*cnp;				//����������S_kl���е�Ԫ�ص�r�к�
	int c = tc - l*cnp;				//����������S_kl���е�Ԫ�ص�c�к�

	//S_kl��(r,c)Ԫ����sum(Y_ik��r�к�W_il��c�еĵ��) for all i=1,..,n3Dpts
	dtype sum = 0;
	dtype3 *ptr1, *ptr2;

	int WYblksize = cnp*pnp;
	int comm3DCnt = comm3DIdxCnt[k*nCams + l];
	int comm3Dbase = k*nCams*n3Dpts + l*n3Dpts;

	///*
	for (int i = 0; i <comm3DCnt; i++)
	{
		idx = comm3DIdx[comm3Dbase + i];		//�õ�3D�������

		Yidx = blk_idx[idx*nCams + k];
		Widx = blk_idx[idx*nCams + l];
		Ybase = Yidx*WYblksize + r*pnp;
		Wbase = Widx*WYblksize + c*pnp;

		//������ʽ
		ptr1 = (dtype3 *)&Yblks[Ybase];
		ptr2 = (dtype3 *)&Wblks[Wbase];
		sum += dot(*ptr1, *ptr2);
	}
	//*/

	/*
	for (int i = 0; i < n3Dpts; i++)
	{
		//�õ�Yik����
		//�õ�Wil����
		Yidx = blk_idx[i*nCams + k];
		Widx = blk_idx[i*nCams + l];
		if (Yidx < 0 || Widx < 0)
			continue;
		Ybase = Yidx*WYblksize + r*pnp;
		Wbase = Widx*WYblksize + c*pnp;
		ptr1 = (dtype3 *)&Yblks[Ybase];
		ptr2 = (dtype3 *)&Wblks[Wbase];
		sum += dot(*ptr1, *ptr2);
	}
	*/

	//����ǶԽǿ飬Uk-Y_ik*W_il^t
	if (k == l)
		sum = U[k*cnp*cnp + r*cnp + c] - sum;
	else
		sum = -sum;
	S[tr*nCams*cnp + tc] = sum;
}