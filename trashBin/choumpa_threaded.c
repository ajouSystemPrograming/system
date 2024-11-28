/*

수업 시간에 그냥 미션으로 구현한 것
실제 사용 가치는 없음.

*/
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define POUT 23 // 초음파센서
#define PIN 24 // 초음파센서
#define POUT2 16 // LED

double distance;

static int GPIOExport(int pin) {
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
	char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return (-1);
	}

	if (-1 ==
			write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
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

		close(fd);
		return (0);
	}
}

void *led_breathing(void *data) {

    pthread_t tid;

    double time;
    clock_t start_t, end_t;
	tid = pthread_self();
    char *thread_name = (char *)data;
    int i = 0;

    while (1) {

        printf("[%s] tid:%x --- %d\n", thread_name, (unsigned int)tid, i);

		if (-1 == GPIOWrite(POUT, 1)) {
			printf("gpio write/trigger err\n");
			pthread_exit(NULL);
		}

		// 1sec == 1000000ultra_sec, 1ms = 1000ultra_sec
		usleep(10);
		GPIOWrite(POUT, 0);

		while (GPIORead(PIN) == 0) {
			start_t = clock();
		}
		while (GPIORead(PIN) == 1) {
			end_t = clock();
		}

		time = (double)(end_t - start_t) / CLOCKS_PER_SEC;  // ms
		printf("time : %.4lf\n", time);
		distance = time / 2 * 34000;
		printf("distance : %.2lfcm\n", distance);

		/* 
		distance에 따른 led 출력 여부 결정
		거리가 5cm ~ 15cm 일 때만 led 점등
		*/
		if (distance < 5.00) {
			printf("Please put it farther\n");
			GPIOWrite(POUT2, 0);
		} else if (distance > 15.00) {
			printf("Please put it closer\n");
			GPIOWrite(POUT2, 0);
		} else {
			printf("Proper distance\n");
			GPIOWrite(POUT2, 1);
            pthread_exit(NULL);
		}
		

		usleep(2000000);
	}
}

int main(int argc, char *argv[]) {
	int repeat = 9;

	// Enable GPIO pins
	if (-1 == GPIOExport(POUT) || -1 == GPIOExport(PIN) || -1 == GPIOExport(POUT2)) {
		printf("gpio export err\n");
		return (1);
	}
	// wait for writing to export file
	usleep(100000);

	// Set GPIO directions
	if (-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(PIN, IN) || -1 == GPIODirection(POUT2, OUT)) {
		printf("gpio direction err\n");
		return (2);
	}

	// init ultrawave trigger
	GPIOWrite(POUT, 0);
	usleep(10000);

	// start
    pthread_t p_thread[2];
	int thr_id;
	int status;
	char p1[] = "thread_1";
	char p2[] = "thread_2";
	char pM[] = "thread_m";
    thr_id = pthread_create(&p_thread[0], NULL, led_breathing, (void *)p1);
	if (thr_id < 0) {
		perror("thread create error : ");
		exit(0);
	}

	thr_id = pthread_create(&p_thread[1], NULL, led_breathing, (void *)p2);
	if (thr_id < 0) {
		perror("thread create error : ");
		exit(0);
	}
    led_breathing((void *)pM);
	pthread_join(p_thread[0], (void **)&status);
	pthread_join(p_thread[1], (void **)&status);

	// Disable GPIO pins
	if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN) || 1 == GPIOUnexport(POUT2)) return (4);

	printf("complete\n");

	return (0);
}
