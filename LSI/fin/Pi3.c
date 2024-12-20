#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

// =========[CAM]===========
#include "camera_capture.h"


// =====[IMAGE PROCESSING]=====
#include "cube.h"
#include "pixel_extract.h"


// ====[SERVO MOTOR CONTROL]====
#define MOTOR_0 1450000
#define MOTOR_90 400000

#include "servo.h"


// =========[MAIN]==========
#include "cube_solving.h"

#define TRUE 1
#define FALSE 0

#define LED 2

#define VALUE_MAX 40
#define DIRECTION_MAX 128
#define IN 0
#define OUT 1

static int GPIOExport(int pin) {
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		exit(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return 0;
}

static int GPIODirection(int pin, int dir) {
	static const char s_directions_str[] = "in\0out";

	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		exit(-1);
	}

	if (-1 ==
			write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		exit(-1);
	}

	close(fd);
	return (0);
}

static int GPIOWrite(int pin, int value) {
	static const char s_values_str[] = "01";

	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		exit(-1);
	}

	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		exit(-1);
	}

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
		exit(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return (0);
}


/* step 1: camera sensing & processing */
int process_cube(int fd) {
	int msg; // received from server 
	int sig; // send to server -> 0010 : this pi's id is 2
	while(TRUE) {
		sig = 2;
		for(pl=0; pl<6; pl++) { // for 6 planes of the cube
			while(TRUE) {
				int t0 = read(sock, &msg, sizeof(msg));
				printf("%d %d\n", t0, msg);
				if(t0 < 0) continue; // misread
				if(t0 == 0) exit(0); // disconnected case - reboot	
				int t1=msg;
				//printf("receive %d, bytes: %d\n", msg, t0);
				if(msg > 0) { // if not, skip step 1
					if(msg==2) { // wait until read the signal
						//printf("signal ok\n");
						break;
					}
					//printf("non zero\n");
				}
				else
					return 0;
			}
			//printf("aaaa\n");
			//system("./cap");
			capture(fd); // capture cube plane image
			usleep(1000);
			GPIOWrite(LED, 1);
			usleep(1);
			process_image(); // get colors for the cube plane and apply to cube planar
			printf("capture & process a plane!\n");
			usleep(3000000);
			GPIOWrite(LED, 0);
			usleep(1);
			write(sock, &sig, sizeof(sig)); // alert captured, ready for next sign
		}

		long long l0 = encode();

		//printf("send! %lld\n", l0);
		sig |= 4; // sig = 0110 -> all planes captured, sending cube planar 
		write(sock, &sig, sizeof(int)); // send signal first
		write(sock, &l0, sizeof(long long)); // send cube planar
	}

}


/* step 2: with socket, TCP communication, do cube solving */
int task(void) {
	pthread_t p_thread[2];
	int thr_id;
	int thr_id2;
	int status;
	char p1[] = "receiving_thread";
	char p2[] = "sending_thread";

	thr_id = pthread_create(&p_thread[0], NULL, receiving_thread, NULL);
	if (thr_id < 0) {
		perror("receiving_thread created error : ");
		exit(0);
		//goto step2;
	}
	thr_id2 = pthread_create(&p_thread[1], NULL, sending_thread, NULL);
	if (thr_id2 < 0) {
		perror("sending_thread created error : ");
		exit(0);
	}

	pthread_join(p_thread[0], (void **)&status);
	pthread_join(p_thread[1], (void **)&status);

	printf("Finished step 2!\n");

	return 0;
}


/* step 3: control servo motor behaviors */
int actuate(void) {
	int msg;
	int sig = 2; // identifier

	while(TRUE) {
		int t0 = read(sock,&msg,sizeof(msg));
		if(t0 == 0) end(); //system("sudo reboot now");
		if(msg==-1) // if step 3 finished
			break;
		if(msg==2) { // if the command is for mine
			printf("read spin message!\n");
			spin(PWM, MOTOR_90);
		}
		write(sock,&sig,sizeof(sig)); // always send signal who am I
	}

	PWMDisable(PWM);
	PWMUnexport(PWM);

	return 0;
}


/* before end */
int end(void) {
	GPIOWrite(LED, 0);
	GPIOUnexport(LED);

	PWMDisable(PWM);
	PWMUnexport(PWM);

	close(sock);

	exit(1);
}


/* main func of client pi 3 */
int main(int argc, char *argv[]) {
	int step;

	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	init_mask();

	GPIOExport(LED);
	usleep(1000000);
	GPIODirection(LED, OUT);
	usleep(1000000);

	PWMExport(PWM);
	PWMWritePeriod(PWM, 10000000);
	PWMWriteDutyCycle(PWM, MOTOR_0);
	PWMEnable(PWM);

	init_socket(argv[1], argv[2]); // network setup
	read(sock, &step, sizeof(int)); // read current step

	switch(step) {
		case 1: goto step1;
		case 2: goto step2;
		case 3: goto step3;
	}

	/* step 1 */
	int fd;
step1:
	fd = init_camera();
	process_cube(fd);

	GPIOUnexport(LED);
	usleep(1);

	close(fd);

step2:
	/* step 2 */
	task();

step3:
	/* step 3 */
	actuate();

	/* fin */
	end();
	//close(sock);
	//system("sudo reboot now");

	return 0;

}
