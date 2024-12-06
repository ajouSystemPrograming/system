//2024.12.04
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
#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define VALUE_MAX 40
#define DIRECTION_MAX 40
#define PI_MAX 1
//test========================================================
static char cube[13][17]={
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,8,7,7,7,2,7,2,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,0,7,0,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,0,7,1,7,3,7,3,7,2,7,4,7,5,7,5,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,0,7,2,7,4,7,1,7,3,7,1,7,4,7,5,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,3,7,4,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,1,7,5,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7}
};
//==========================================================test
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
int toint[100];
char* tochar = "RGBYOW* EXYZ";
int move[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
int frame[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
int order[24][2] = {{5,1},{5,3},{7,1},{7,3},{5,5},{5,7},{7,5},{7,7},{5,9},{5,11},{7,9},{7,11},{5,13},{5,15},{7,13},{7,15},{1,5},{1,7},{3,5},{3,7},{9,5},{9,7},{11,5},{11,7}};
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
void addmap(long long cur, long long pre, int rot){
    node *n1 = hashmap[cur%100003]->next;
    node *n2 = (node *)malloc(sizeof(node));
    hashmap[cur%100003]->next = n2;
    n2->next = n1;
    n2->value = cur;
    n2->pre = pre;
    n2->rot = rot;
}
void printCube(int r, int c) {
    int i0,i1;
    for(i0 = 0; i0 < 4; i0++)
        cube[r+frame[i0][0]][c+frame[i0][1]]=6;
    for(i0 = 0; i0 < 13; i0++)
        printf("\033[A");
    for(i0 = 0; i0 < 13; i0++)
    {
        for(i1 = 0; i1 < 17; i1++)
            printf("%c",tochar[cube[i0][i1]]);
        printf("\n");
    }
    for(i0 = 0; i0 < 4; i0++)
        cube[r+frame[i0][0]][c+frame[i0][1]]=7;
}
//
void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
static int GPIOExport(int pin) {
    #define BUFFER_MAX 3
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open export for writing!\n");
        return (-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
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
static int init(){
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
    
    if (GPIOWrite(UPOUT, 1) == -1) {
        return 3;
    }
    if (GPIOWrite(DOWNOUT, 1) == -1) {
        return 3;
    }
    if (GPIOWrite(LEFTOUT, 1) == -1) {
        return 3;
    }
    if (GPIOWrite(RIGHTOUT, 1) == -1) {
        return 3;
    }
    if (GPIOWrite(SOUT, 1) == -1) {
        return 3;
    }
    
    //
    int i0;
    mask[0] = 1;
    for(i0 = 1; i0 < 24; i0++)
        mask[i0] = mask[i0-1]*6;
    for(int i0 = 0; i0 < 100003; i0++)
        hashmap[i0] = (node *)malloc(sizeof(node));
        
    toint['R'] = 0;
    toint['G'] = 1;
    toint['B'] = 2;
    toint['Y'] = 3;
    toint['O'] = 4;
    toint['W'] = 5;
    
    return 0;
}
static int end(){
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
static int ButtonInput(){
    
    int buttonState = 0;
    buttonState += (1^GPIORead(UPIN));
    
    buttonState += (1^GPIORead(DOWNIN))<<1;
    
    buttonState += (1^GPIORead(LEFTIN))<<2;
    
    buttonState += (1^GPIORead(RIGHTIN))<<3;
    
    buttonState += (1^GPIORead(SIN))<<4;
    return buttonState;
}
static int getColor(long long l0, int index) {
    return (l0/mask[index])%6;
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
long long back;
int use[6] = {0};
long long answer[6] = {0};
int qp = 0;
void make(int x) {
    int i0, i1;
    if(x==3)
    {
        answer[qp++] = encode();
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
int serv_sock, clnt_sock[3] = {-1,-1,-1};
struct sockaddr_in serv_addr, clnt_addr[3], clnt_addr[3];
socklen_t clnt_addr_size[3];
long long buffer[100000][50];
int head = 0, tail = 0;
long long msg[150];
int fin = 0, start = 0;
int receive[3];
int piindex[PI_MAX][10000000];
int st[PI_MAX], en[PI_MAX];
void process2(int num) {
    int i0, i1;
    
    for(i0 = 0; i0 < 150; i0++)
        if(!contains(msg[i0])) {
            addmap(msg[i0],buffer[piindex[num][st[num]]][i0/3],i0%3);
            for(i1 = 0; i1 < qp; i1++)
                if(msg[i0]==answer[i1]) {
                    back = msg[i0];
                    fin = 1;
                    buffer[head/50][0]=-1;
                    for(i0 = 0; i0 < PI_MAX; i0++)
                        write(clnt_sock[i0], buffer[head/50], sizeof(buffer[0]));
                    printf("FIN\n");
                    return;
                }
			buffer[tail/50][tail%50] = msg[i0];
            tail++;
		}
    st[num]++;
    //printf("head: %d tail: %d\n",head,tail);
    int t0 = write(clnt_sock[num], buffer[head/50], sizeof(buffer[0]));
    piindex[num][en[num]++] = head/50;
    head = head/50*50+50;
    if(tail<head)
    {
        exit(0);
        printf("error");
        tail = head;
    }
}
int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        return 1;
    }
    //pthread_t receiveThread[3], sendThread[3];
    //int thr_id;
    int t0, pre, r = 5, c = 1, preState=0, buttonState=0;
    int i0, i1;
    long long l0=4179031333688006790, l1;
    if(init())
        return 1;
    decode(l0);
    //network
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) error_handling("socket() error");
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1) error_handling("listen() error");
    for(i0 = 0; i0 < PI_MAX; i0++)
    {        
        if (clnt_sock[i0] < 0) {
            clnt_addr_size[i0] = sizeof(clnt_addr[i0]);
            clnt_sock[i0] = accept(serv_sock, (struct sockaddr *)&clnt_addr[i0], &clnt_addr_size[i0]);
            if (clnt_sock[i0] == -1) error_handling("accept() error");
        }
        int flag = fcntl(clnt_sock[i0], F_GETFL);
        //int flag = fcntl(serv_sock, F_GETFL);
        fcntl(clnt_sock[i0], F_SETFL, flag | O_NONBLOCK);
        printf("Connection established %d %d\n", O_NONBLOCK,flag);
    }
    //
    for(int i = 0; i < 13; i++)
    {
        for(int j = 0; j < 17; j++)
            printf("%c",tochar[cube[i][j]]);
        printf("\n");
    }
    while(1)//change cube using button
    {
        buttonState = ButtonInput();
        int print = 0;
        if(!(preState & (1 << 4)) && buttonState & (1 << 4))
        {
            if(cube[r][c]<6)
            {
                cube[r][c] = (cube[r][c]+1) % 6;
                print = 1;
            }
            else if(cube[r][c]==8)
                break;
        }
        else
        {
            for(i0 = 0; i0 < 4; i0++)
                if(!(preState & (1 << i0)) && buttonState & (1 << i0)){
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
    buffer[0][tail++] = encode();
    addmap(encode(),-1,0);
    
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
    for(i0 = 0; i0 < 2000; i0++)
    {
        l0 = buffer[head/50][head%50];
        head++;
		l1 = X(l0);
		if(!contains(l1))
		{
            for(i1 = 0; i1 < qp; i1++)
                if(l1==answer[i1])
                    break;
            if(i1!=qp)
            {
                fin = 1;
                break;
            }
			addmap(l1, l0, 0);
			buffer[tail/50][tail%50] = l1;
            tail++;
		}
		
		l1 = Y(l0);
		if(!contains(l1))
		{
            for(i1 = 0; i1 < qp; i1++)
                if(l1==answer[i1])
                    break;
            if(i1!=qp)
            {
                fin = 1;
                break;
            }
			addmap(l1, l0, 1);
			buffer[tail/50][tail%50] = l1;
            tail++;
		}
		
		l1 = Z(l0);
		if(!contains(l1))
		{
            for(i1 = 0; i1 < qp; i1++)
                if(l1==answer[i1])
                    break;
            if(i1!=qp)
            {
                fin = 1;
                break;
            }
			addmap(l1, l0, 2);
			buffer[tail/50][tail%50] = l1;
            tail++;
		}
    }
    
    if(!fin) {
        clock_t t = clock();
        for(i0 = 0; i0 < 10; i0++) {
            for(i1 = 0; i1 < PI_MAX; i1++) {
                t0 = write(clnt_sock[i1], buffer[head/50], sizeof(buffer[0]));
                piindex[i1][en[i1]++] = head/50;
                head+=50;
            }
        }
        t0=0;
        while(!fin)
            for(i0 = 0; i0 < PI_MAX && !fin; i0++) {
                t0 = read(clnt_sock[i0], msg, sizeof(msg));
                //printf("read Len : %d\n", t0);
                if(t0 > 0) {
                    //printf("t0: %d %d %lld\n",i0, t0, sizeof(msg));
                    process2(i0);
                }
            }
        printf("\ntime: %lf\n", (double)(clock()-t)/CLOCKS_PER_SEC);
        l0 = back;
        printf("END %lld\n",l0);
        printf(" %lld\n",contains(l0)->pre);
    }
    //serve
    int stack[100];
    int sp = 0;
    while(contains(l0)->pre>0)
    {
        stack[sp++] = contains(l0)->rot;
        l0 = contains(l0)->pre;
    }
    while(sp-->0)
    {
        printf("%c",tochar[stack[sp]+9]);
    }
    
    //
    return end();
}
