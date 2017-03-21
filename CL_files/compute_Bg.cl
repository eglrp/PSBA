
/*
����Bga=U*ga+W*gb
*/
__kernel void kern_compute_Bg(		//const int cnp,
	//const int pnp,
	const int nCams,
	const int n3Dpts,
	__global int *blk_idx,
	__global dtype *g,
	__global dtype *U,
	__global dtype *V,
	__global dtype *Wblks,
	__global dtype *Bg)
{ 
	dtype3 *ptr1, *ptr2;
	dtype3 *ptr3, *ptr4;
	int Widx, UVWbase, gbase;
	int r = get_global_id(0);
	private dtype sum = 0.0;
	int WYblksize = cnp*pnp;

	if(r<nCams*cnp) 
	{
		int j = (int)(r / cnp);	//��Ӧ��Uj
		int rj = r - j*cnp;		//��Ӧ��Uj,W_ij�ĵ�rj�У� ͬʱָʾBga_j�ĵ�rj��Ԫ��

		//***** ����U_j�ĵ�rj����ga_j�ĳ˻�
		UVWbase = j*cnp*cnp + rj*cnp;
		gbase = j*cnp;
		sum = U[UVWbase++] * g[gbase++];
		sum += U[UVWbase++] * g[gbase++];
		sum += U[UVWbase++] * g[gbase++];
		sum += U[UVWbase++] * g[gbase++];
		sum += U[UVWbase++] * g[gbase++];
		sum += U[UVWbase] * g[gbase];

		//*****����W_ij�ĵ�rj����gb_i�ĳ˻�
		for (int i = 0;i < n3Dpts;i++)
		{
			Widx = blk_idx[i*nCams + j];		//W_ij�Ĵ洢����
			if (Widx < 0) continue;
			ptr1 = (dtype3 *)&Wblks[Widx*WYblksize + rj*pnp];	//W_ij�ĵ�rj�еĵ�ַ
			ptr2 = (dtype3 *)&g[nCams*cnp + pnp*i];			//gb_i�ĵ�ַ
			sum += dot(*ptr1, *ptr2);
		}
	}
	else {
		int br = r - nCams*cnp;
		int i = (int)br / pnp;	//��Ӧ��Vi
		int ri = br - i*pnp;		//��Ӧ��Vi�ĵ�ri��, (W_ij)^T��ri�У�W_ij��ri�У��� ͬʱָʾBgb_i�ĵ�ri��Ԫ��

		//*****����(W_ij)^T�ĵ�ri����ga_j�ĳ˻�
		for (int j = 0;j < nCams;j++)
		{
			Widx = blk_idx[i*nCams + j];		//W_ij�Ĵ洢����
			if (Widx < 0) continue;
			UVWbase = Widx*WYblksize + ri;			//W_ij��(0,ri)
			gbase = j*cnp;
			sum += Wblks[UVWbase] * g[gbase++];
			UVWbase += pnp;							//W_ij��(1,ri)
			sum += Wblks[UVWbase] * g[gbase++];
			UVWbase += pnp;							//W_ij��(2,ri)
			sum += Wblks[UVWbase] * g[gbase++];
			UVWbase += pnp;							//W_ij��(3,ri)
			sum += Wblks[UVWbase] * g[gbase++];
			UVWbase += pnp;							//W_ij��(4,ri)
			sum += Wblks[UVWbase] * g[gbase++];
			UVWbase += pnp;							//W_ij��(5,ri)
			sum += Wblks[UVWbase] * g[gbase];
		}

		//����Vi��ri����gb_i�ĳ˻�
		UVWbase = i*pnp*pnp + ri*pnp;
		gbase = nCams*cnp + i*pnp;
		sum += V[UVWbase++] * g[gbase++];
		sum += V[UVWbase++] * g[gbase++];
		sum += V[UVWbase] * g[gbase];
	}

	Bg[r] = sum;

}