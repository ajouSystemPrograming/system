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

#endif
