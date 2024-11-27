#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define IN 0
#define OUT 1

#define LOW 0
#define HIGH 1

#define VALUE_MAX 40
#define DIRECTION_MAX 40

//test========================================================
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
    {7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7}
};
//==========================================================test

//
#define UPPIN 17
#define UPOUT 27
#define DOWNPIN 20
#define DOWNOUT 21
#define LEFTPIN 6
#define LEFTOUT 13
#define RIGHTPIN 19
#define RIGHTOUT 26

char* tochar = "RGBYOW* ";
int move[4][2] = {{-1,0},{1,0},{0,-1},{0,-1}};
int frame[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
void printCube(int r, int c)
{
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
//


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
        fprintf(stderr, "Failed to open gpio direction for writing!\n");
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
        fprintf(stderr, "Failed to open gpio value for reading!\n");
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
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return (-1);
    }

    if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
        fprintf(stderr, "Failed to write value!\n");
        return (-1);
    }

    close(fd);
    return (0);
}
//int len = 8;
//int usePinNum[100] = {UPIN,UPOUT,DOWNIN,DOWNOUT,LEFTIN,LEFTOUT,RIGHTIN,RIGHTOUT};
static int init(){
    if (GPIOExport(UPOUT) == -1 || GPIOExport(UPPIN)) {
        return 1;
    }
    if (GPIODirection(UPOUT, OUT) == -1 || GPIODirection(UPPIN, IN) == -1) {
        return 2;
    }
    if (GPIOExport(DOWNOUT) == -1 || GPIOExport(DOWNPIN)) {
        return 1;
    }
    if (GPIODirection(DOWNOUT, OUT) == -1 || GPIODirection(DOWNPIN, IN) == -1) {
        return 2;
    }
    if (GPIOExport(LEFTOUT) == -1 || GPIOExport(LEFTPIN)) {
        return 1;
    }
    if (GPIODirection(LEFTOUT, OUT) == -1 || GPIODirection(LEFTPIN, IN) == -1) {
        return 2;
    }
    if (GPIOExport(RIGHTOUT) == -1 || GPIOExport(RIGHTPIN)) {
        return 1;
    }
    if (GPIODirection(RIGHTOUT, OUT) == -1 || GPIODirection(RIGHTPIN, IN) == -1) {
        return 2;
    }

    printCube(r, c);
    return 0;
}
static int end(){
    if (GPIOUnexport(UPOUT) == -1 || GPIOUnexport(UPPIN) == -1) {
        return 4;
    }
    if (GPIOUnexport(DOWNOUT) == -1 || GPIOUnexport(DOWNPIN) == -1) {
        return 4;
    }
    if (GPIOUnexport(LEFTOUT) == -1 || GPIOUnexport(RIGHTPIN) == -1) {
        return 4;
    }
    if (GPIOUnexport(RIGHTOUT) == -1 || GPIOUnexport(RIGHTPIN) == -1) {
        return 4;
    }

    return 0;
}

static int ButtonInput(){
    int state = 1;
    int prev_state = 1;
    int light = 0;
    
    int buttonState = 0;
    if (GPIOWrite(UPOUT2, 1) == -1) {
        return 3;
    }
    buttonState += GPIORead(UPIN);
    buttonState += GPIORead(DOWNIN)<<1;
    buttonState += GPIORead(LEFTIN)<<2;
    buttonState += GPIORead(RIGHTIN)<<3;
    printf("BUTTONSTATE: U:%d D:%d L:%d R:%d\n", buttonState,buttonState&(1),buttonState&(1<<2),buttonState&(1<<3));
    return buttonState;
}

int main(int argc, char* argv[]) {
    
    if(!init())
        return 1;

    int repeat = 100;
    int t0, pre, r = 5, c = 1;
    while(repeat-->0)
    {
        t0 = ButtonInput();
        for(i0 = 0; i0 < 4; i0++)
            if((1<<i0)&t0!=0){
                r += move[i0][0];
                c += move[i0][1];
            }
        if(r<1)
            r = 1;
        else if(r>11)
            r = 11;
        if(c<1)
            c = 1;
        else if(c>15)
            c = 15;
        printCube(r, c);
        usleep(1000 * 1000);//1s
    }

    return end();
}