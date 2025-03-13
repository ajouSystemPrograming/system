//2024.12.08
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
#include <signal.h>

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

#define VALUE_MAX 40
#define DIRECTION_MAX 40
#define ARR_MAX 10
#define PI_MAX 3
#define LIMIT 100000000

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

static char cube[13][17]={
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,8,7,7,7,1,7,1,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,1,7,1,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,3,7,5,7,4,7,4,7,5,7,0,7,3,7,4,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,4,7,0,7,3,7,2,7,0,7,1,7,5,7,2,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,1,7,5,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,3,7,0,7,7,7,7,7,7,7,7,7},
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7}
};

int toint[100];//012345
char* tochar =  "RGBYOW* EXYZ";
int move[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
int frame[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
int order[24][2] = {{5,1},{5,3},{7,1},{7,3},{5,5},{5,7},{7,5},{7,7},{5,9},{5,11},{7,9},{7,11},{5,13},{5,15},{7,13},{7,15},{1,5},{1,7},{3,5},{3,7},{9,5},{9,7},{11,5},{11,7}};

static long long mask[24];

typedef struct {
    int rot;
    long long value, pre;
    struct node* next;
} node;
//해시맵 구현
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
//해시맵 add, contain
void addmap(long long cur, long long pre, int rot){
    node *n1 = hashmap[cur%100003]->next;
    node *n2 = (node *)malloc(sizeof(node));
    hashmap[cur%100003]->next = n2;
    n2->next = n1;
    n2->value = cur;
    n2->pre = pre;
    n2->rot = rot;
}

//큐브 전개도 출력
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

//오류처리
void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
//GPIO 관련
static int GPIOExport(int pin) {
    char buffer[3];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open export for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, sizeof(buffer), "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}
static int GPIOUnexport(int pin) {
    char buffer[3];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open unexport for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, sizeof(buffer), "%d", pin);
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
//초기화
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

    //비트마스크 초기화
    int i0;
    mask[0] = 1;
    for(i0 = 1; i0 < 24; i0++)
        mask[i0] = mask[i0-1]*6;
    //해시 맵 초기화
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
//자원 반환 및 종료
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
//버튼 입력
static int ButtonInput() {

    int buttonState = 0;
    buttonState += (1^GPIORead(UPIN));

    buttonState += (1^GPIORead(DOWNIN))<<1;

    buttonState += (1^GPIORead(LEFTIN))<<2;

    buttonState += (1^GPIORead(RIGHTIN))<<3;

    buttonState += (1^GPIORead(SIN))<<4;
    return buttonState;
}
//전개도 index면에서 색 추출
static int getColor(long long l0, int index) {
    return (l0/mask[index])%6;
}
//전개도 -> long long 변환
static long long encode() {
    int i0;
    long long l0 = 0;
    for(i0 = 0; i0 < 24; i0++)
        l0 += cube[order[i0][0]][order[i0][1]]*mask[i0];
    return l0;
}
//long long -> 전개도 변환
static void decode(long long l0) {
    int i0;
    for(i0 = 0; i0 < 24; i0++)
        cube[order[i0][0]][order[i0][1]] = getColor(l0, i0);
}
//x축 회전
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
//y축 회전
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
//z축 회전
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

int step;
long long back;
int use[6];
long long answer[6];
int qp = 0;
//입력된 전개도에서 가능한 해 전개도 계산
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
long long buffer[5000000/ARR_MAX][ARR_MAX];
int head, tail;

long long msg[ARR_MAX*4];

int fin, start;
int piindex[PI_MAX][5000000];
int st[PI_MAX], en[PI_MAX];
//초기화
static void reset() {
    step = 1;
    head = 0;
    tail = 0;
    qp = 0;
    fin = 0;
    start = 0;
    for(int i0 = 0; i0 < 100003; i0++)
        hashmap[i0] = (node *)malloc(sizeof(node));
    for(int i0 = 0; i0 < PI_MAX; i0++) {
        st[i0] = 0;
        en[i0] = 0;
    }
    for(int i0 = 0; i0 < 3; i0++)
        use[i0] = 0;
}
//서브파이와 연결
static int connectPI(int num) {
    int i0, t0;
    if (clnt_sock[num] < 0) {
        clnt_addr_size[num] = sizeof(clnt_addr[num]);
        clnt_sock[num] = accept(serv_sock, (struct sockaddr *)&clnt_addr[num], &clnt_addr_size[num]);
        if (clnt_sock[num] == -1) return 0;
        int flag = fcntl(clnt_sock[num], F_GETFL);
        fcntl(clnt_sock[num], F_SETFL, flag | O_NONBLOCK);
        write(clnt_sock[num], &step, sizeof(step));
        printf("Connection established\n");
    }
    return 1;
}
//서브파이로부터 데이터 받으면 해시맵에 추가하고 새로운 데이터를 서브파이한테 보냄
int process2(int num) {
    int i0, i1;
    //받은 데이터가 어느 전개도로부터 나온 데이터인지 추적하고 중복되지 않으면 map, buffer에 추가
    for(i0 = 0; i0 < ARR_MAX*4; i0++) {
        if(i0%4==0)
            continue;
        else if(!contains(msg[i0])) {
            addmap(msg[i0],msg[i0/4*4],(i0%4)-1);
            for(i1 = 0; i1 < qp; i1++)
                if(msg[i0]==answer[i1]) {
                    back = msg[i0];
                    fin = 1;
                    buffer[head/ARR_MAX][0]=-1;
                    for(i0 = 0; i0 < PI_MAX; i0++)
                        write(clnt_sock[i0], buffer[head/ARR_MAX], sizeof(buffer[0]));
                    return 1;
                }
			buffer[tail/ARR_MAX][tail%ARR_MAX] = msg[i0];
            tail++;
		}
    }
    st[num]++;
    //서브파이에게 새로 계산해야할 데이터를 전송
    int t0 = write(clnt_sock[num], buffer[head/ARR_MAX], sizeof(buffer[0]));
    piindex[num][en[num]++] = head/ARR_MAX;
    head = head/ARR_MAX*ARR_MAX+ARR_MAX;
    //불가능한 전개도일 경우
    if(tail<head)
    {
        printf("IMPOSSIBLE CUBE TRY AGAIN!\n");
        tail = head;
        for(i0 = 0; i0 < PI_MAX; i0++) {
            clnt_sock[i0] = -1;
            close(clnt_sock[i0]);
        }
        return -1;
    }
    return 0;
}
//강제종료 시 자원반환
void ctrlC(int sig) {
    end();
	char msg[100];
    for(int i0 = 0; i0 < PI_MAX; i0++)
		if(clnt_sock[i0] > 0) {
			close(clnt_sock[i0]);
		}
	close(serv_sock);
    exit(0);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, ctrlC);
    //서브파이 연결 종료 시 오류 무시
    signal(SIGPIPE, SIG_IGN);
    if(argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        return 1;
    }
    int t0, pre, r = 5, c = 1, preState=0, buttonState=0;
    int i0, i1;
    long long l0=4271069981394162510, l1;
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
    t0 = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &t0, sizeof(t0)); //포트 재사용으로 binderror 무시
    if(bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("bind() error");
    }
    if (listen(serv_sock, PI_MAX) == -1) error_handling("listen() error");

    int flag = fcntl(serv_sock, F_GETFL);
    fcntl(serv_sock, F_SETFL, flag | O_NONBLOCK);
    for(i0 = 0; i0 < PI_MAX; i0++) {
        connectPI(i0);
    }

start:
    reset();
    //step1: 입력
    for(int i = 0; i < 13; i++)
    {
        for(int j = 0; j < 17; j++)
            printf("%c",tochar[cube[i][j]]);
        printf("\n");
    }

    int c0, count = 0, capture = 1<<2;
    int capcount = 2;
    while(1) {//버튼 입력 및 서브파이와의 통신
        buttonState = ButtonInput();
        int print = 0;
        for(i0 = 0; i0 < PI_MAX; i0++) {
            //서브파이 연결 재시도
            if(clnt_sock[i0] == -1 && !connectPI(i0)){
                capture |= 1<<2;
                continue;
            }
            t0 = read(clnt_sock[i0], &c0, sizeof(c0));
            if(t0 > 0) {
                //printf("c0: %d\n",c0);
                switch(c0&3) {
                    case 1:
                        t0 = -1;
                        while(t0 < 0) {
                            t0 = read(clnt_sock[i0], &c0, sizeof(c0));
                        }
                        if(t0 == 0) {
                            clnt_sock[i0] = -1;
                            close(clnt_sock[i0]);
                            break;
                        }
                        //printf("get 1 %d\n",c0);
                        if(capture&(1<<1))
                            capture -= 1<<1;
                        capture += (c0>>2)<<1;
                        break;
                    case 2:
                        if(c0==2) {
                            capture |= 1<<2;
                            break;
                        }
                        t0 = -1;
                        while(t0 < 0) {
                            t0 = read(clnt_sock[i0], &l0, sizeof(l0));
                        }
                        if(t0 == 0) {
                            clnt_sock[i0] = -1;
                            close(clnt_sock[i0]);
                            capture |= 1<<2;
                            break;
                        }
                        //printf("get 2 %lld\n",l0);
                        decode(l0);
                        print = 1;
                        break;
                    case 3:
                        t0 = -1;
                        while(t0 < 0) {
                            t0 = read(clnt_sock[i0], &c0, sizeof(c0));
                        }
                        if(t0 == 0) {
                            clnt_sock[i0] = -1;
                            close(clnt_sock[i0]);
                            break;
                        }
                        if(capture&(1<<3))
                            capture -= 1<<3;
                        capture += (c0>>2)<<3;
                        break;
                }
            }
            else if(t0==0) {//eof 받을 시 처리
                clnt_sock[i0] = -1;
                close(clnt_sock[i0]);
            }
        }

        //촬영 가능한 환경일 경우 촬영 요청
        if(capture == 0b1110) {
            capture -= 1<<2;
            c0 = 2;
            for(i0 = 0; i0 < PI_MAX; i0++)
                write(clnt_sock[i0], &c0, sizeof(c0));
        }
        //선택버튼 처리
        if(!(preState & (1 << 4)) && buttonState & (1 << 4)) {
            if(cube[r][c]<6) {
                cube[r][c] = (cube[r][c]+1) % 6;
                print = 1;
            }
            else if(cube[r][c]==8)
                break;
        }
        else {//조이스틱 버튼 처리
            for(i0 = 0; i0 < 4; i0++)
                if(!(preState & (1 << i0)) && buttonState & (1 << i0)) {
                    r += move[i0][0]<<1;
                    c += move[i0][1]<<1;
                    print = 1;
                }
            if(r<1) r = 1; else if(r>11) r = 11;
            if(c<1) c = 1; else if(c>15) c = 15;
        }
        //변경사항 있을 시 전개도 출력
        if(print)
            printCube(r, c);
        preState = buttonState;
        usleep(500*100);
    }
    //입력 끝

    //버퍼에 전개도 넣기
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
    //입력된 전개도에서 가능한 해 전개도 계산
    make(0);
    //서브파이에 다음단계로 넘어감을 알림
    c0 = -1;
    for(i0 = 0; i0 < PI_MAX; i0++) {
        write(clnt_sock[i0], &c0, sizeof(c0));
    }
    //step2: 분산처리
    step = 2;
    //완성된 전개도를 입력받으면 바로 종료
    for(i1 = 0; i1 < qp; i1++)
        if(buffer[0][head]==answer[i1]) {
            printf("already complete\n");
            long long tmp[10];
            tmp[0] = -2;
            for(i0 = 0; i0 < PI_MAX; i0++)
                if(clnt_sock[i0]>0)
                    write(clnt_sock[i0],tmp,sizeof(tmp));
            goto start;//초기화
        }
    //서브 파이로 보낼 데이터 생성
    for(i0 = 0; i0 < 2000; i0++) {
        l0 = buffer[head/ARR_MAX][head%ARR_MAX];
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
			buffer[tail/ARR_MAX][tail%ARR_MAX] = l1;
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
			buffer[tail/ARR_MAX][tail%ARR_MAX] = l1;
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
			buffer[tail/ARR_MAX][tail%ARR_MAX] = l1;
            tail++;
		}
    }

    if(!fin) {
        clock_t t = clock();
        //서브파이마다 버퍼에 5개의 배열을 버퍼로 넣음
        for(i0 = 0; i0 < 5; i0++) {
            for(i1 = 0; i1 < PI_MAX; i1++)
                if(clnt_sock[i1] > 0) {
                    t0 = write(clnt_sock[i1], buffer[head/ARR_MAX], sizeof(buffer[0]));
                    piindex[i1][en[i1]++] = head/ARR_MAX;
                    head+=ARR_MAX;
            }
        }
        t0=0;
        while(!fin)
            for(i0 = 0; i0 < PI_MAX && !fin; i0++) {
                //연산 도중 연결되는 파이한테 데이터 보냄
                if(clnt_sock[i0] == -1) {
                    if(!connectPI(i0)) {
                        continue;
                    }
                    for(i1 = 0; i1 < 5; i1++) {
                        if(clnt_sock[i0] > 0) {
                            t0 = write(clnt_sock[i0], buffer[head/ARR_MAX], sizeof(buffer[0]));
                            piindex[i0][en[i0]++] = head/ARR_MAX;
                            head+=ARR_MAX;
                        }
                    }
                }
                t0 = read(clnt_sock[i0], msg, sizeof(msg));
                int t1 = 0;
                int t2 = t1;
                int t3 = t2;
                //서브파이로부터 데이터를 받으면 처리 후 보내는 과정 반복
                if(t0 > 0) {
                    if(process2(i0)<0) {
                        t0 = -2;
                        long long tmp[10];
                        tmp[0] = -2;
                        for(i0 = 0; i0 < PI_MAX; i0++)
                            write(clnt_sock[i0],tmp,sizeof(tmp));
                        goto start;
                    }
                }
                else if(t0==0) {//연결 종료 시 데이터 복구
                    for(i1 = st[i0]; i1 < en[i0]; i1++)
                        for(int i2 = 0; i2 < ARR_MAX; i2++) {
                            buffer[head/ARR_MAX][head%ARR_MAX] = buffer[piindex[i0][i1]][i2];
                            head++;
                        }
                    en[i0] = st[i0];
                    clnt_sock[i0] = -1;
                    close(clnt_sock[i0]);
                }
            }
        l0 = back;
    }
    //분산처리 종료

    //step3: 동작
    usleep(1000*1000);
    step = 3;
    char tmp;
    //소켓 버퍼 초기화
    for(i0 = 0; i0 < PI_MAX; i0++) {
        while(read(clnt_sock[i0], &tmp, sizeof(tmp))>0);
        if(clnt_sock[i0] > 0) {
            int flag = fcntl(clnt_sock[i0], F_GETFL);
            fcntl(clnt_sock[i0], F_SETFL, flag & ~O_NONBLOCK);
        }
    }
    int stack[100];
    int sp = 0;
    //역추적으로 path 찾기
    while(contains(l0)->pre>0) {
        stack[sp++] = contains(l0)->rot;
        l0 = contains(l0)->pre;
    }
    int canserv = 1;
    //끊긴 파이 있는지 확인
    for(i0 = 0; i0 < PI_MAX; i0++) {
        if(clnt_sock[i0] == -1) {
            canserv = 0;
            break;
        }
    }
    while(sp-->0) {
        c0 = stack[sp]+1;
        //방향 출력
        printf("%c",tochar[stack[sp]+9]);
        //끊긴 파이가 없다면 서보모터 회전
        if(canserv) {
            printf(" Spin\n");
            for(i0 = 0; i0 < PI_MAX; i0++) {
                if(clnt_sock[i0] == -1) {
                    canserv = 0;
                    break;
                }
                t0 = write(clnt_sock[i0], &c0, sizeof(c0));
            }
            for(i0 = 0; i0 < PI_MAX; i0++) {
                if(clnt_sock[i0] == -1) {
                    canserv = 0;
                    break;
                }
                t0 = read(clnt_sock[i0], &c0, sizeof(c0));
                if(t0 == 0) {
                    canserv = 0;
                    break;
                }
            }
        }
    }
    //서브파이에게 종료를 알림
    c0 = -1;
    for(i0 = 0; i0 < PI_MAX; i0++)
        t0 = write(clnt_sock[i0], &c0, sizeof(c0));
    usleep(3000*1000);
    goto start;//초기화

    return end();
}