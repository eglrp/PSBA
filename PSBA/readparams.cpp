#include "stdafx.h"

#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "readparams.h"
#include "psba.h"

#define NOCOV     0
#define FULLCOV   1
#define TRICOV    2

#define MAXSTRLEN  2048 /* 2K */

/* get rid of the rest of a line upto \n or EOF */
inline void SKIP_LINE(FILE *fp){
	char buf[MAXSTRLEN];															\
	while (!feof(fp))																\
		if (!fgets(buf, MAXSTRLEN - 1, fp) || buf[strlen(buf) - 1] == '\n') break;   \
}




/* 
��ȡ���������
* ����ļ��е�ÿһ�ж�Ӧһ�����
*/
static int findNcameras(FILE *fp)
{
	int lineno, ncams, ch;

	lineno = ncams = 0;
	while (!feof(fp)) {
		if ((ch = fgetc(fp)) == '#') { /* skip comments */
			SKIP_LINE(fp);
			++lineno;
			continue;
		}
		if (feof(fp)) break;
		ungetc(ch, fp);
		SKIP_LINE(fp);
		++lineno;
		if (ferror(fp)) {
			fprintf(stderr, "findNcameras(): error reading input file, line %d\n", lineno);
			exit(1);
		}
		++ncams;
	}
	return ncams;
}


/* reads (from "fp") "nvals" doubles into "vals".
* Returns number of doubles actually read, EOF on end of file, EOF-1 on error
*/
static int readNDoubles(FILE *fp, dtype *vals, int nvals)
{
	register int i;
	int n, j;

	for (i = n = 0; i<nvals; ++i) {
		if(sizeof(dtype)==4)
			j = fscanf_s(fp, "%f", vals + i);
		else 
			j = fscanf_s(fp, "%lf", vals + i);
		if (j == EOF) return EOF;

		if (j != 1 || ferror(fp)) return EOF - 1;

		n += j;
	}

	return n;
}

/* 
reads (from "fp") "nvals" doubles without storing them.
* Returns EOF on end of file, EOF-1 on error
*/
static int skipNDoubles(FILE *fp, int nvals)
{
	register int i;
	int j;

	for (i = 0; i<nvals; ++i) {
		j = fscanf_s(fp, "%*f");
		if (j == EOF) return EOF;

		if (ferror(fp)) return EOF - 1;
	}

	return nvals;
}

/* 
��ȡnvals��������vals�С�
����ʵ�ʶ�ȡ�������ĸ�������EOF/EOF-1
*/
static int readNInts(FILE *fp, int *vals, int nvals)
{
	register int i;
	int n, j;

	for (i = n = 0; i<nvals; ++i) {
		j = fscanf_s(fp, "%d", vals + i);
		if (j == EOF) return EOF;

		if (j != 1 || ferror(fp)) return EOF - 1;

		n += j;
	}

	return n;
}

/* 
�����ı��ļ��е�һ�е�double��ֵ������. rewinds file.
*/
static int countNDoubles(FILE *fp)
{
	int lineno, ch, np, i;
	char buf[MAXSTRLEN], *s;
	dtype dummy;
	double tmp;

	lineno = 0;
	while (!feof(fp)) {
		if ((ch = fgetc(fp)) == '#') { /* skip comments */
			SKIP_LINE(fp);
			++lineno;
			continue;
		}
		if (feof(fp)) return 0;
		ungetc(ch, fp);
		++lineno;
		if (!fgets(buf, MAXSTRLEN - 1, fp)) { /* read the line found... */
			fprintf(stderr, "countNDoubles(): error reading input file, line %d\n", lineno);
			exit(1);
		}
		/* ...and count the number of doubles it has */
		for (np = i = 0, s = buf; 1; ++np, s += i) {
			if (sizeof(dtype) == 4) {
				ch = sscanf_s(s, "%lf%n", &tmp, &i);
				dummy = (dtype)tmp;
			}
			else
				ch = sscanf_s(s, "%lf%n", &dummy, &i);
			if (ch == 0 || ch == EOF) break;
		}
		rewind(fp);
		return np;
	}
	return 0; // should not reach this point
}


/*
��ȡ�������,
���������
	cnp				-->�������ά��
	infilter����	-->���ڽ���Ԫ���Ϊ��ת����
	filecnp			-->
�����
	params			-->��Ÿ�����Ĳ���

*/
static void readCameraParams(FILE *fp, int cnp,
	void(*infilter)(dtype *pin, int nin, dtype *pout, int nout), int filecnp,
	dtype *params, dtype *initrot)
{
	int lineno, n, ch;
	dtype *tofilter;

	if (filecnp>0 && infilter) {
		if ((tofilter = (dtype *)malloc(filecnp * sizeof(dtype))) == NULL) {
			;
			fprintf(stderr, "memory allocation failed in readCameraParams()\n");
			exit(1);
		}
	}
	else { // camera params will be used as read
		infilter = NULL;
		tofilter = NULL;
		filecnp = cnp;
	}
	/* make sure that the parameters file contains the expected number of parameters per line */
	if ((n = countNDoubles(fp)) != filecnp) {
		fprintf(stderr, "readCameraParams(): expected %d camera parameters, first line contains %d!\n", filecnp, n);
		exit(1);
	}

	lineno = 0;
	while (!feof(fp)) {
		if ((ch = fgetc(fp)) == '#') { /* skip comments */
			SKIP_LINE(fp);
			++lineno;
			continue;
		}

		if (feof(fp)) break;

		ungetc(ch, fp);
		++lineno;
		if (infilter) {
			n = readNDoubles(fp, tofilter, filecnp);
			(*infilter)(tofilter, filecnp, params, cnp);
		}
		else
			n = readNDoubles(fp, params, cnp);
		if (n == EOF) break;
		if (n != filecnp) {
			fprintf(stderr, "readCameraParams(): line %d contains %d parameters, expected %d!\n", lineno, n, filecnp);
			exit(1);
		}
		if (ferror(fp)) {
			fprintf(stderr, "findNcameras(): error reading input file, line %d\n", lineno);
			exit(1);
		}

		/* save rotation assuming the last 3 parameters correspond to translation */
		initrot[1] = params[cnp - 6];
		initrot[2] = params[cnp - 5];
		initrot[3] = params[cnp - 4];
		initrot[0] = sqrt(1.0 - initrot[1] * initrot[1] - initrot[2] * initrot[2] - initrot[3] * initrot[3]);

		params += cnp;
		initrot += FULLQUATSZ;
	}
	if (tofilter) free(tofilter);
}


/*
�������ļ��У���һ�ж�Ӧһ��3D�㣬�Լ����ڸ�����ϵ�ͶӰ�㡣
* X_0...X_{pnp-1}  nframes  frame0 x0 y0 [cov0]  frame1 x1 y1 [cov1] ...
* The portion of the line starting at "frame0" is ignored for all but the first line
���������
	fp			-->�������ļ�
	mnp			-->2D���ά�ȣ�һ��Ϊ2
�����
	n3Dpts		-->��3D������
	nprojs		-->��ͶӰ2D������
	havecov		-->�Ƿ���Э����
*/
static void readNpointsAndNprojections(FILE *fp, int *n3Dpts, int pnp, int *nprojs, int mnp, int *havecov)
{
	int nfirst, lineno, npts, nframes, ch, n;

	/* #parameters for the first line */
	nfirst = countNDoubles(fp);
	*havecov = NOCOV;

	*n3Dpts = *nprojs = lineno = npts = 0;
	while (!feof(fp)) {
		if ((ch = fgetc(fp)) == '#') { /* ���������#��ͷ���������� */
			SKIP_LINE(fp);
			++lineno;
			continue;
		}
		if (feof(fp)) break;
		ungetc(ch, fp);
		++lineno;
		skipNDoubles(fp, pnp);	//����pnp��doubleֵ
		n = readNInts(fp, &nframes, 1);	//��ȡ3D���Ӧ֡����
		if (n != 1) {
			fprintf(stderr, "readNpointsAndNprojections(): error reading input file, line %d: "
				"expecting number of frames for 3D point\n", lineno);
			exit(1);
		}
		if (npts == 0) { /* ����һ�У����Ƿ���Э���� */
			nfirst -= (pnp + 1); /* ��ȥpnp��֡���ʣ�µ���֡��ż�֡�϶�Ӧ2D�� */
			if (nfirst == nframes*(mnp + 1 + mnp*mnp)) { /* full mnpxmnp covariance */
				*havecov = FULLCOV;
			}
			else if (nfirst == nframes*(mnp + 1 + mnp*(mnp + 1) / 2)) { /* ������ʽ�����Э���� */
				*havecov = TRICOV;
			}
			else {
				*havecov = NOCOV;
			}
		}
		SKIP_LINE(fp);
		*nprojs += nframes;		//��ͶӰ�����ۼ�
		++npts;
	}

	*n3Dpts = npts;
}


/* reads the number of (double) parameters contained in the first non-comment row of a file */
int readNumParams(char *fname)
{
	FILE *fp;
	int n;

	if ( fopen_s(&fp,fname, "r") != 0) {
		fprintf(stderr, "error opening file %s!\n", fname);
		exit(1);
	}

	n = countNDoubles(fp);
	fclose(fp);

	return n;
}

/* 
�ӵ������ļ���ȡ3D�㼰��ӦͶӰ��
���룺


�����
	parmas		-->3D��
	projs		-->2DͶӰ��

* "params", "projs" & "vmask" are assumed preallocated, pointing to
* memory blocks large enough to hold the parameters of 3D points,
* their projections in all images and the point visibility mask, respectively.
* Also, if "covprojs" is non-NULL, it is assumed preallocated and pointing to
* a memory block suitable to hold the covariances of image projections.
* Each 3D point is assumed to be defined by pnp parameters and each of its projections
* by mnp parameters. Optionally, the mnp*mnp covariance matrix in row-major order
* follows each projection. All parameters are stored in a single line.
*
* �ļ���ʽ is X_{0}...X_{pnp-1}  nframes  frame0 x0 y0 [covx0^2 covx0y0 covx0y0 covy0^2] frame1 x1 y1 [covx1^2 covx1y1 covx1y1 covy1^2] ...
* with the parameters in angle brackets being optional. To save space, only the upper
* triangular part of the covariance can be specified, i.e. [covx0^2 covx0y0 covy0^2], etc
*/
static void readPointParamsAndProjections(FILE *fp, dtype *params, int pnp, dtype *projs, dtype *covprojs,
	int havecov, int mnp, char *vmask, int ncams)
{
	int nframes, ch, lineno, ptno, frameno, n;
	int ntord, covsz = mnp*mnp, tricovsz = mnp*(mnp + 1) / 2, nshift;
	register int i, ii, jj, k;

	lineno = ptno = 0;
	while (!feof(fp)) {
		if ((ch = fgetc(fp)) == '#') { /* skip comments */
			SKIP_LINE(fp);
			lineno++;
			continue;
		}
		if (feof(fp)) break;
		ungetc(ch, fp);
		n = readNDoubles(fp, params, pnp); /* read in point parameters */
		if (n == EOF) break;
		if (n != pnp) {
			fprintf(stderr, "readPointParamsAndProjections(): error reading input file, line %d:\n"
				"expecting %d parameters for 3D point, read %d\n", lineno, pnp, n);
			exit(1);
		}
		params += pnp;

		n = readNInts(fp, &nframes, 1);  /* read in number of image projections */
		if (n != 1) {
			fprintf(stderr, "readPointParamsAndProjections(): error reading input file, line %d:\n"
				"expecting number of frames for 3D point\n", lineno);
			exit(1);
		}

		for (i = 0; i<nframes; ++i) {
			n = readNInts(fp, &frameno, 1); /* read in frame number... */
			if (frameno >= ncams) {
				fprintf(stderr, "readPointParamsAndProjections(): line %d contains an image projection for frame %d "
					"but only %d cameras have been specified!\n", lineno + 1, frameno, ncams);
				exit(1);
			}

			n += readNDoubles(fp, projs, mnp); /* ...and image projection */
			projs += mnp;
			if (n != mnp + 1) {
				fprintf(stderr, "readPointParamsAndProjections(): error reading image projections from line %d [n=%d].\n"
					"Perhaps line contains fewer than %d projections?\n", lineno + 1, n, nframes);
				exit(1);
			}

			if (covprojs != NULL) {
				if (havecov == TRICOV) {
					ntord = tricovsz;
				}
				else {
					ntord = covsz;
				}
				n = readNDoubles(fp, covprojs, ntord); /* read in covariance values */
				if (n != ntord) {
					fprintf(stderr, "readPointParamsAndProjections(): error reading image projection covariances from line %d [n=%d].\n"
						"Perhaps line contains fewer than %d projections?\n", lineno + 1, n, nframes);
					exit(1);
				}
				if (havecov == TRICOV) {
					/* complete the full matrix from the triangular part that was read.
					* First, prepare upper part: element (ii, mnp-1) is at position mnp-1 + ii*(2*mnp-ii-1)/2.
					* Row ii has mnp-ii elements that must be shifted by ii*(ii+1)/2
					* positions to the right to make space for the lower triangular part
					*/
					for (ii = mnp; --ii; ) {
						k = mnp - 1 + ((ii*((mnp << 1) - ii - 1)) >> 1); //mnp-1 + ii*(2*mnp-ii-1)/2
						nshift = (ii*(ii + 1)) >> 1; //ii*(ii+1)/2;
						for (jj = 0; jj<mnp - ii; ++jj) {
							covprojs[k - jj + nshift] = covprojs[k - jj];
							//covprojs[k-jj]=0.0; // this clears the lower part
						}
					}
					/* copy to lower part */
					for (ii = mnp; ii--; )
						for (jj = ii; jj--; )
							covprojs[ii*mnp + jj] = covprojs[jj*mnp + ii];
				}
				covprojs += covsz;
			}

			vmask[ptno*ncams + frameno] = 1;
		}

		fscanf_s(fp, "\n"); // consume trailing newline

		lineno++;
		ptno++;
	}
}



/* 
���ǰ��ĺ�������txt�ļ���ȡ��ʼ��SfM��Ϣ
ͬʱ��Ҳ��ȡ3D���ͼ��ͶӰ�������ѡ����ʹ����ΪNULL��
����4��������Ҫ��̬�ط����ڴ档

���룺
	camsfname	-->��������ļ�
	ptsfname	-->3D�㼰��Ӧ2DͶӰ����ļ�
	infilter	-->����Ԫ���Ϊ��ת����ĺ���
	cnp,pnp,mnp	-->�ֱ��ʾ�������ά�ȣ�3D��ά�ȣ��Լ�2D��ά��
	filecnp		-->ָ����������ļ���ÿ�������ά��
�����
	ncams		-->�������
	n3Dpts		-->3D������
	n2Dprojs	-->2DͶӰ������

*/
void readInitialSBAEstimate(char *camsfname, char *ptsfname, int cnp, int pnp, int mnp,
	void(*infilter)(dtype *pin, int nin, dtype *pout, int nout), int filecnp,
	int *ncams, int *n3Dpts, int *n2Dprojs,
	dtype **motstruct, dtype **initrot, dtype **imgpts, dtype **covimgpts, char **vmask)
{
	FILE *fpc, *fpp;
	int havecov;

	//��ȡ����ļ��͵��ļ�
	if (fopen_s(&fpc,camsfname, "r") != 0) {
		fprintf(stderr, "cannot open file %s, exiting\n", camsfname);
		system("pause");
		exit(1);
	}
	if (fopen_s(&fpp, ptsfname, "r") != 0) {
		fprintf(stderr, "cannot open file %s, exiting\n", ptsfname);
		system("pause");
		exit(1);
	}
	//��ȡ���������
	*ncams = findNcameras(fpc);
	//��ȡ3D���Լ�2DͶӰ���������n3Dpts��n2Dprojs
	readNpointsAndNprojections(fpp, n3Dpts, pnp, n2Dprojs, mnp, &havecov);

	//Ϊ���������3D�����洢��
	*motstruct = (dtype *)malloc((*ncams*cnp + *n3Dpts*pnp) * sizeof(dtype));
	if (*motstruct == NULL) {
		fprintf(stderr, "memory allocation for 'motstruct' failed in readInitialSBAEstimate()\n");
		system("pause");
		exit(1);
	}
	//Ϊ�����Ԫ������洢��
	*initrot = (dtype *)malloc((*ncams*FULLQUATSZ) * sizeof(dtype)); // Note: this assumes quaternions for rotations!
	if (*initrot == NULL) {
		fprintf(stderr, "memory allocation for 'initrot' failed in readInitialSBAEstimate()\n");
		system("pause");
		exit(1);
	}
	//Ϊ����2D�����洢��
	*imgpts = (dtype *)malloc(*n2Dprojs*mnp * sizeof(dtype));
	if (*imgpts == NULL) {
		fprintf(stderr, "memory allocation for 'imgpts' failed in readInitialSBAEstimate()\n");
		system("pause");
		exit(1);
	}
	if (havecov) {
		*covimgpts = (dtype *)malloc(*n2Dprojs*mnp*mnp * sizeof(dtype));
		if (*covimgpts == NULL) {
			fprintf(stderr, "memory allocation for 'covimgpts' failed in readInitialSBAEstimate()\n");
			system("pause");
			exit(1);
		}
	}
	else
		*covimgpts = NULL;
	//Ϊ���������洢������Ϊһ��3D�㲻����ͶӰ������ͼ��֡�ϡ�
	*vmask = (char *)malloc(*n3Dpts * *ncams * sizeof(char));
	if (*vmask == NULL) {
		fprintf(stderr, "memory allocation for 'vmask' failed in readInitialSBAEstimate()\n");
		system("pause");
		exit(1);
	}
	memset(*vmask, 0, *n3Dpts * *ncams * sizeof(char)); /* clear vmask */

	/* prepare for re-reading files */
	rewind(fpc);
	rewind(fpp);

	//��ȡ��������� motstruct��initrot
	readCameraParams(fpc, cnp, infilter, filecnp, *motstruct, *initrot);
	//��ȡ3D�㵽motstruct��3D�洢���֣���ȡ2D�㼰���뵽imgpts��vmask�С�����2D���Э�����ȡ��covimgpts�С�
	readPointParamsAndProjections(fpp, *motstruct + *ncams*cnp, pnp, *imgpts, *covimgpts, havecov, mnp, *vmask, *ncams);

	fclose(fpc);
	fclose(fpp);
}