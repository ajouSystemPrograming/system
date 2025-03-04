#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1
#define DIRECTION_MAX 256
#define VALUE_MAX 256

#define SIN 17
#define SOUT 27
#define UPIN 19
#define UPOUT 26
#define DOWNIN 20
#define DOWNOUT 21
#define LEFTIN 6
#define LEFTOUT 13
#define RIGHTIN 2
#define RIGHTOUT 3
#define BUFFER_MAX 5000000

//test========================================================
static char cube[13][17]={
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,8,7,7,7,4,7,3,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,4,7,3,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,0,7,0,7,1,7,4,7,2,7,2,7,5,7,3,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,0,7,0,7,1,7,4,7,2,7,2,7,5,7,3,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,5,7,1,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,5,7,1,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7}
};
//adding
int inc[100];
int dec[6] = {'R','G','B','Y','O','W'};
//
char* tochar = "RGBYOW* EXYZ";
int move[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
int frame[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
int order[24][2] = {{5,1},{5,3},{7,1},{7,3},{5,5},{5,7},{7,5},{7,7},{5,9},{5,11},{7,9},{7,11},{5,13},{5,15},{7,13},{7,15},{1,5},{1,7},{3,5},{3,7},{9,5},{9,7},{11,5},{11,7}};
long long inputBuffer[BUFFER_MAX];
long long outputBuffer[BUFFER_MAX];
int inputHead = 0, inputTail = 0;
int outputHead = 0, outputTail = 0;

static long long mask[24];
typedef struct {
    int rot;
    long long value, pre;
    struct node* next;
} node;
node *hashmap[100003];

node *contains(long long l0) {
    node *tmp = hashmap[l0%100003];
    while(tmp->next)
    {
        tmp = tmp->next;
        if(tmp->value == l0)
            return tmp;
    }
    return 0;
}

void addmap(long long cur, long long pre, int rot) {
    node *n1 = hashmap[cur%100003]->next;
    node *n2 = (node *)malloc(sizeof(node));
    hashmap[cur%100003]->next = n2;
    n2->next = n1;
    n2->value = cur;
    n2->pre = pre;
    n2->rot = rot;
}
//
void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

static int GPIOExport(int pin) {
    char buffer[3];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open export for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, 3, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}

static int GPIOUnexport(int pin) {
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open unexport for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);

    close(fd);
    return (0);
}

static int GPIODirection(int pin, int dir) {
    static const char s_directions_str[] = "in\0out";
    char path[DIRECTION_MAX];
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio%d direction for writing!\n", pin);
        return (-1);
    }

    if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
        fprintf(stderr, "Failed to set direction!\n");
        return (-1);
    }

    close(fd);
    return (0);
}

static int GPIORead(int pin) {
    char path[VALUE_MAX];
    char value_str[3];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio%d value for reading!\n", pin);
        return (-1);
    }

    if (-1 == read(fd, value_str, 3)) {
        fprintf(stderr, "Failed to read value!\n");
        return (-1);
    }

    close(fd);
    return (atoi(value_str));
}

static int GPIOWrite(int pin, int value) {
    static const char s_values_str[] = "01";

    char path[VALUE_MAX];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio%d value for writing!\n", pin);
        return (-1);
    }

    if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
        fprintf(stderr, "Failed to write value!\n");
        return (-1);
    }

    close(fd);
    return (0);
}

static int init() {
    if (GPIOExport(SOUT) == -1 || GPIOExport(SIN)) {
        return 1;
    }
    usleep(100000);
    if (GPIODirection(SOUT, OUT) == -1 || GPIODirection(SIN, IN) == -1) {
        return 2;
    }
    if (GPIOExport(UPOUT) == -1 || GPIOExport(UPIN)) {
        return 1;
    }
    usleep(100000);
    if (GPIODirection(UPOUT, OUT) == -1 || GPIODirection(UPIN, IN) == -1) {
        return 2;
    }
    if (GPIOExport(DOWNOUT) == -1 || GPIOExport(DOWNIN)) {
        return 1;
    }
    usleep(100000);
    if (GPIODirection(DOWNOUT, OUT) == -1 || GPIODirection(DOWNIN, IN) == -1) {
        return 2;
    }
    if (GPIOExport(LEFTOUT) == -1 || GPIOExport(LEFTIN)) {
        return 1;
    }
    usleep(100000);
    if (GPIODirection(LEFTOUT, OUT) == -1 || GPIODirection(LEFTIN, IN) == -1) {
        return 2;
    }
    if (GPIOExport(RIGHTOUT) == -1 || GPIOExport(RIGHTIN)) {
        return 1;
    }
    usleep(100000);
    if (GPIODirection(RIGHTOUT, OUT) == -1 || GPIODirection(RIGHTIN, IN) == -1) {
        return 2;
    }
    //
    int i0;
    mask[0] = 1;
    for(i0 = 1; i0 < 24; i0++)
        mask[i0] = mask[i0-1]*6;
    for(int i0 = 0; i0 < 100003; i0++)
        hashmap[i0] = (node *)malloc(sizeof(node));
    
    //add
    inc['R'] = 0;
    inc['G'] = 1;
    inc['B'] = 2;
    inc['Y'] = 3;
    inc['O'] = 4;
    inc['W'] = 5;
    //
    return 0;
}

static int end() {
    if (GPIOUnexport(SOUT) == -1 || GPIOUnexport(SIN) == -1) {
        return 4;
    }
    if (GPIOUnexport(UPOUT) == -1 || GPIOUnexport(UPIN) == -1) {
        return 4;
    }
    if (GPIOUnexport(DOWNOUT) == -1 || GPIOUnexport(DOWNIN) == -1) {
        return 4;
    }
    if (GPIOUnexport(LEFTOUT) == -1 || GPIOUnexport(LEFTIN) == -1) {
        return 4;
    }
    if (GPIOUnexport(RIGHTOUT) == -1 || GPIOUnexport(RIGHTIN) == -1) {
        return 4;
    }

    return 0;
}

static int ButtonInput() {
    
    int buttonState = 0;
    if (GPIOWrite(UPOUT, 1) == -1) {
        return 3;
    }
    buttonState += (1^GPIORead(UPIN));
    
    if (GPIOWrite(DOWNOUT, 1) == -1) {
        return 3;
    }
    buttonState += (1^GPIORead(DOWNIN))<<1;
    
    if (GPIOWrite(LEFTOUT, 1) == -1) {
        return 3;
    }
    buttonState += (1^GPIORead(LEFTIN))<<2;
    
    if (GPIOWrite(RIGHTOUT, 1) == -1) {
        return 3;
    }
    buttonState += (1^GPIORead(RIGHTIN))<<3;
    if (GPIOWrite(SOUT, 1) == -1) {
        return 3;
    }
    buttonState += (1^GPIORead(SIN))<<4;
    return buttonState;
}

void printCube(int r, int c) {
    int i0,i1;
    for(i0 = 0; i0 < 4; i0++)
        cube[r+frame[i0][0]][c+frame[i0][1]]=6;
    
    for(i0 = 0; i0 < 13; i0++)
    {
        for(i1 = 0; i1 < 17; i1++)
            printf("%c",tochar[cube[i0][i1]]);
        printf("\n");
    }
    for(i0 = 0; i0 < 4; i0++)
        cube[r+frame[i0][0]][c+frame[i0][1]]=7;
}

static int getColor(long long l0, int index) {
    return (l0/mask[index])%6;
}

static void setColor(long long l0, int index, int color) {
    l0 -= ((l0 / mask[index]) % 6) * mask[index];
    l0 += color*mask[index];
}
static long long encode() {
    int i0;
    long long l0 = 0;
    for(i0 = 0; i0 < 24; i0++)
        l0 += cube[order[i0][0]][order[i0][1]]*mask[i0];
    return l0;
}

static void decode(long long l0) {
    int i0;
    for(i0 = 0; i0 < 24; i0++)
        cube[order[i0][0]][order[i0][1]] = getColor(l0, i0);
}

long long X(long long l0) {
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

long long Y(long long l0) {
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

long long Z(long long l0) {
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

int use[6] = {0};
long long q[6] = {0};
int qp = 0;
void make(int x)
{
    int i0, i1;
    if(x==3)
    {
        q[qp++] = encode();
        return;
    }
    for(i0 = 0; i0 < 6; i0++)
        if(!use[i0])
        {
            use[i0]=1;
            int t0=x==0?4:x==1?8:16;
            for(i1 = 0; i1 < 4; i1++)
                cube[order[t0+i1][0]][order[t0+i1][1]]=i0;
            make(x+1);
            use[i0]=0;
        }
}

int main(int argc, char *argv[]) {
    int pre, r = 5, c = 1, preState=0, buttonState=0;
    int i0, i1, t0, t1;
    long long l0 = 4271069981394162510;
    int status, thr_id;
	if(init())
        return 1;
    decode(l0);
    printCube(5,1);
    /*
    while(1){//change cube using button
        buttonState = ButtonInput();
        int print = 0;
        if(!(preState & (1 << 4)) && buttonState & (1 << 4)) {
            if(cube[r][c]<6){
                cube[r][c] = (cube[r][c]+1) % 6;
                print = 1;
            }
            else if(cube[r][c]==8)
                break;
        }
        else{
            for(i0 = 0; i0 < 4; i0++)
                if(!(preState & (1 << i0)) && buttonState & (1 << i0)) {
                    r += move[i0][0]<<1;
                    c += move[i0][1]<<1;
                    print = 1;
                }
            if(r<1) r = 1; else if(r>11) r = 11;
            if(c<1) c = 1; else if(c>15) c = 15;
        }
        if(print)
            printCube(r, c);
        preState = buttonState;
        usleep(500*100);
    }
    */
    outputBuffer[outputTail++] = encode();
    addmap(encode(),-1,0);
    printf("%lld\n",encode());
    //
    
    for(i0 = 0; i0 < 4; i0++)
    {
        cube[order[i0][0]][order[i0][1]] = cube[order[2][0]][order[2][1]];
        cube[order[i0+12][0]][order[i0+12][1]] = cube[order[15][0]][order[15][1]];
        cube[order[i0+20][0]][order[i0+20][1]] = cube[order[22][0]][order[22][1]];
    }
    use[cube[order[0][0]][order[0][1]]] = 1;
    use[cube[order[12][0]][order[12][1]]] = 1;
    use[cube[order[20][0]][order[20][1]]] = 1;
    make(0);
    printf("ING %d\n",qp);
    clock_t t = clock();
    long long l1;
    int count = 0;
    while(outputHead<BUFFER_MAX)
    {
		count++;
        l0 = outputBuffer[outputHead++];
		l1 = X(l0);
		if(!contains(l1))
		{
			addmap(l1, l0, 0);
			outputBuffer[outputTail++] = l1;
		}
        for(i0 = 0; i0 < qp; i0++)
			if(l1==q[i0])
				break;
		if(i0!=qp)
			break;
		
		l1 = Y(l0);
		if(!contains(l1))
		{
			addmap(l1, l0, 1);
			outputBuffer[outputTail++] = l1;
		}
        for(i0 = 0; i0 < qp; i0++)
			if(l1==q[i0])
				break;
		if(i0!=qp)
			break;
		
		l1 = Z(l0);
		if(!contains(l1))
		{
			addmap(l1, l0, 2);
			outputBuffer[outputTail++] = l1;
		}
        for(i0 = 0; i0 < qp; i0++)
			if(l1==q[i0])
				break;
		if(i0!=qp)
			break;
		//if(outputHead%1000==0)
		//printf("%d %d\n",outputHead, outputTail);
    }
    printf("\ntime: %lf", (double)(clock()-t)/CLOCKS_PER_SEC);
    printf("count: %d\n", count);
    while(contains(l0)->pre>0)
    {
        printf("%c", tochar[(contains(l0)->rot)+9]);
        l0 = contains(l0)->pre;
    }
    
}
