#pragma once

#include "cl_psba.h"
#include "psba.h"

int levmar(
	//����
	PSBA_structPtr psbaPtr,						//
	int cnp, int pnp, int mnp,		//�ֱ����������ά�ȣ�3D������ά�ȣ�2Dͼ�������ά��
	int n3Dpts, int nCams, int n2Dprojs,				//3D�����������������2Dͼ�������
	dtype *finalErr);