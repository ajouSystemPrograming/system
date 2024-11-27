#include <stdio.h>

char cube[13][17]={
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,4,7,4,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,4,7,4,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,0,7,0,7,1,7,1,7,2,7,2,7,3,7,3,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,0,7,0,7,1,7,1,7,2,7,2,7,3,7,3,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,5,7,5,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,5,7,5,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
};
char* tochar = "RGBYOW* ";
int dir[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
void printCube(int r, int c)
{
    int i0,i1;
    for(i0 = 0; i0 < 4; i0++)
        cube[r+dir[i0][0]][c+dir[i0][1]]=6;
    
    for(i0 = 0; i0 < 13; i0++)
    {
        for(i1 = 0; i1 < 17; i1++)
            printf("%c",tochar[cube[i0][i1]]);
        printf("\n");
    }
    for(i0 = 0; i0 < 4; i0++)
        cube[r+dir[i0][0]][c+dir[i0][1]]=7;
}
int main(int argc, char* argv[]) {
    

    int repeat = 100;
    int i0, i1;
    int t0, pre, r = 5, c = 1;
    printCube(r, c);
    
    while(repeat-->0)
    {
        scanf("%d",&t0);
        switch(t0)
        {
            case 8:
                r-=2;
                break;
            case 5:
                r+=2;
                break;
            case 4:
                c-=2;
                break;
            case 6:
                c+=2;
                break;
            default:
                if(cube[r][c]==7)
                    break;
                cube[r][c]++;
                if(cube[r][c]==6)
                    cube[r][c]=0;
        }
        if(r<0)
            r = 1;
        else if(r>11)
            r = 11;
        if(c<0)
            c = 1;
        else if(c>15)
            c = 15;
        printCube(r, c);
    }
    return 0;
}
