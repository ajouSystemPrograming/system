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
#include "servo.h"


// =========[MAIN]==========
#include "cube_solving.h"

#define TRUE 1
#define FALSE 0



/* step 1: camera sensing & processing */
int process_cube(int fd) {
	int msg; // received from server 
	int sig = 2; // send to server -> 0010 : this pi's id is 2

	for(pl=0; pl<6; pl++) { // for 6 planes of the cube
		while(TRUE) {
			int t0 = read(sock, &msg, sizeof(msg));
			//printf("%d\n", t0);
			if(t0 < 0) continue; // misread
			if(t0 == 0) system("sudo reboot now"); // disconnected case - reboot	
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
		process_image(); // get colors for the cube plane and apply to cube planar
		printf("capture & process a plane!\n");
		usleep(3000000);
		write(sock, &sig, sizeof(sig)); // alert captured, ready for next sign
	}

	long long l0 = encode();

	//printf("send! %lld\n", l0);
	sig += 4; // sig = 0110 -> all planes captured, sending cube planar 
	write(sock, &sig, sizeof(int)); // send signal first
	write(sock, &l0, sizeof(long long)); // send cube planar
	while(1) {
		int t2 = read(sock, &msg, sizeof(msg));
		if(t2 == 0) system("sudo reboot now"); // server closed - reboot
		if(msg < 0)
			break;

	}

	return 0;
}


/* step 2: with socket, TCP communication, do cube solving */
int task(void) {
	/*
	   char msg;

	   while(TRUE) {
	   if(msg=="OK") { // wait until read the signal
	   break;	// do something
	   }
	   }
	   */

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

	PWMExport(PWM);
	PWMWritePeriod(PWM, 10000000);
	PWMWriteDutyCycle(PWM, 400000);
	PWMEnable(PWM);

	while(TRUE) {
		int t0 = read(sock,&msg,sizeof(msg));
		if(t0 == 0) system("sudo reboot now");
		if(msg==-1) // if step 3 finished
			break;
		if(msg==2) { // if the command is for mine
			//printf("read spin message!\n");
			spin(PWM, 1450000);
		}
		write(sock,&sig,sizeof(sig)); // always send signal who am I
	}

	PWMDisable(PWM);
	PWMUnexport(PWM);

	return 0;
}


/* before stop */
int stop(void) {
	close(sock);
	return 0;
}

void ctrlC(int sig) {
	char msg[100];
	if(sock > 0) {
		close(sock);
		exit(0);
	}
}

	int fd;

/* main func of client pi 3 */
int main(int argc, char *argv[]) {
	int step;

	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	signal(SIGINT, ctrlC);
	
	init_mask();

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
	close(fd);

step2:
	/* step 2 */
	task();

step3:
	/* step 3 */
	actuate();

	/* fin */
	stop();
	//close(sock);
	//system("sudo reboot now");
	
	return 0;
	
}
