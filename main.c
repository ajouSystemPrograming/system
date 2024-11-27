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

//
#define UPPIN 17
#define UPOUT 27
#define DOWNPIN 20
#define DOWNOUT 21
#define LEFTPIN 6
#define LEFTOUT 13
#define RIGHTPIN 19
#define RIGHTOUT 26

int buttonState = 0;
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
//
//int len = 8;
//int usePinNum[100] = {UPIN,UPOUT,DOWNIN,DOWNOUT,LEFTIN,LEFTOUT,RIGHTIN,RIGHTOUT};
static int init()
{
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

    return 0;
}
static int end()
{

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
    int repeat = 100;
    int state = 1;
    int prev_state = 1;
    int light = 0;

    do {
        if (GPIOWrite(UPOUT2, 1) == -1) {
            return 3;
        }
        buttonState=0;
        buttonState += GPIORead(UPIN);
        buttonState += GPIORead(DOWNIN)<<1;
        buttonState += GPIORead(LEFTIN)<<2;
        buttonState += GPIORead(RIGHTIN)<<3;
        printf("BUTTONSTATE: U:%d D:%d L:%d R:%d\n", buttonState,buttonState&(1),buttonState&(1<<2),buttonState&(1<<3));
        usleep(1000 * 1000);//1s
    } while (repeat--);
}
//

int main(int argc, char* argv[]) {
    
    if(!init())
        return 1;

    ButtonInput();



    return end();
}