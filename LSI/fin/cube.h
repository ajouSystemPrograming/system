#ifndef __CUBE_H__
#define __CUBE_H__

/* cube planar array */
static char cube[13][17] = {
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,8,7,7,7,5,7,1,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,4,7,3,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,2,7,5,7,2,7,0,7,1,7,5,7,4,7,0,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,4,7,4,7,1,7,0,7,2,7,1,7,5,7,2,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,3,7,3,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,3,7,0,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7}
};

/* get each point's coordinate */
int order[24][2] = {{5,1},{5,3},{7,1},{7,3},{5,5},{5,7},{7,5},{7,7},{5,9},{5,11},{7,9},{7,11},{5,13},{5,15},{7,13},{7,15},{1,5},{1,7},{3,5},{3,7},{9,5},{9,7},{11,5},{11,7}};

int frame[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};

static long long mask[24];

char* tochar = "RGBYOW* EXYZ";

void init_mask(void) {
	int i0;
	mask[0] = 1;
	for(i0 = 1; i0 < 24; i0++)
		mask[i0] = mask[i0-1]*6;
}

/* encode cube planar array info to long long int */
static long long encode() {
	int i0;
	long long l0 = 0;
	for(i0 = 0; i0 < 24; i0++)
		l0 += cube[order[i0][0]][order[i0][1]]*mask[i0];
	return l0;
}

static int getColor(long long l0, int index) {
	return (l0/mask[index])%6;
}

/* decode long long int to cube planar array info */
static void decode(long long l0) {
	int i0;
	for(i0 = 0; i0 < 24; i0++)
		cube[order[i0][0]][order[i0][1]] = getColor(l0, i0);
}

/* print cube graphically to stdout */
void printCube(int r, int c) {
	int i0,i1;
	for(i0 = 0; i0 < 4; i0++)
		cube[r+frame[i0][0]][c+frame[i0][1]]=6;

	for(i0 = 0; i0 < 13; i0++)
	{
		for(i1 = 0; i1 < 17; i1++)
			printf("%c",tochar[cube[i0][i1]]);
	}
}


long long X(long long l0){
	long long l1 = (l0/mask[5])%6, l2 = (l0/mask[7])%6, l3, l4;
	l0-=l1*mask[5]; l0-=l2*mask[7];
	l0+=(l3=(l0/mask[17])%6)*mask[5]; l0+=(l4=(l0/mask[19])%6)*mask[7];
	l0-=l3*mask[17]; l0-=l4*mask[19];
	l0+=(l3=(l0/mask[14])%6)*mask[17]; l0+=(l4=(l0/mask[12])%6)*mask[19];
	l0-=l3*mask[14]; l0-=l4*mask[12];
	l0+=(l3=(l0/mask[23])%6)*mask[12]; l0+=(l4=(l0/mask[21])%6)*mask[14];
	l0-=l3*mask[23]; l0-=l4*mask[21];
	l0+=l1*mask[21]; l0+=l2*mask[23];
	//
	l1 = (l0/mask[8])%6; l0-=l1*mask[8];
	l0+=(l2=(l0/mask[9])%6)*mask[8]; l0-=l2*mask[9];
	l0+=(l2=(l0/mask[11])%6)*mask[9]; l0-=l2*mask[11];
	l0+=(l2=(l0/mask[10])%6)*mask[11]; l0-=l2*mask[10];
	l0+=l1*mask[10];
	return l0;
}

long long Y(long long l0){
	long long l1 = (l0/mask[0])%6, l2 = (l0/mask[1])%6, l3, l4;
	l0-=l1*mask[0]; l0-=l2*mask[1];
	l0+=(l3=(l0/mask[12])%6)*mask[0]; l0+=(l4=(l0/mask[13])%6)*mask[1];
	l0-=l3*mask[12]; l0-=l4*mask[13];
	l0+=(l3=(l0/mask[8])%6)*mask[12]; l0+=(l4=(l0/mask[9])%6)*mask[13];
	l0-=l3*mask[8]; l0-=l4*mask[9];
	l0+=(l3=(l0/mask[4])%6)*mask[8]; l0+=(l4=(l0/mask[5])%6)*mask[9];
	l0-=l3*mask[4]; l0-=l4*mask[5];
	l0+=l1*mask[4]; l0+=l2*mask[5];
	//
	l1 = (l0/mask[16])%6; l0-=l1*mask[16];
	l0+=(l2=(l0/mask[17])%6)*mask[16]; l0-=l2*mask[17];
	l0+=(l2=(l0/mask[19])%6)*mask[17]; l0-=l2*mask[19];
	l0+=(l2=(l0/mask[18])%6)*mask[19]; l0-=l2*mask[18];
	l0+=l1*mask[18];
	return l0;
}

long long Z(long long l0){
	long long l1 = (l0/mask[1])%6, l2 = (l0/mask[3])%6, l3, l4;
	l0-=l1*mask[1]; l0-=l2*mask[3];
	l0+=(l3=(l0/mask[19])%6)*mask[1]; l0+=(l4=(l0/mask[18])%6)*mask[3];
	l0-=l3*mask[19]; l0-=l4*mask[18];
	l0+=(l3=(l0/mask[10])%6)*mask[19]; l0+=(l4=(l0/mask[8])%6)*mask[18];
	l0-=l3*mask[10]; l0-=l4*mask[8];
	l0+=(l3=(l0/mask[20])%6)*mask[10]; l0+=(l4=(l0/mask[21])%6)*mask[8];
	l0-=l3*mask[20]; l0-=l4*mask[21];
	l0+=l1*mask[20]; l0+=l2*mask[21];
	//
	l1 = (l0/mask[4])%6; l0-=l1*mask[4];
	l0+=(l2=(l0/mask[5])%6)*mask[4]; l0-=l2*mask[5];
	l0+=(l2=(l0/mask[7])%6)*mask[5]; l0-=l2*mask[7];
	l0+=(l2=(l0/mask[6])%6)*mask[7]; l0-=l2*mask[6];
	l0+=l1*mask[6];
	return l0;
}

#endif
