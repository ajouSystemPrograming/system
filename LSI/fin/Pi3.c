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


// =========[MAIN]==========
#include "cube_solving.h"

#define TRUE 1
#define FALSE 0

#define BUFMAX 100


// buffer array
char buff[BUFMAX];


/* step 1: camera sensing & processing */
int process_cube(void) {
	char msg; 

	for(pl=0; pl<6; pl++) { // for 6 planes of the cube
		while(TRUE) {
			int t0 = read(sock, &msg, sizeof(msg));
			if(t0 < 0) continue; // misread
			if(t0 == 0); // disconnected case - reboot	
			if(msg=='o') { // wait until read the signal
				break;
			}
		}
		
		capture(); // capture cube plane image
		process_image(); // get colors for the cube plane and apply to cube planar

		write(sock, &buffer, sizeof(char)); // alert ready for next sign

	}

	long long l0 = encode();

	write(sock, &l0, sizeof(long long));

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
		perror("reveiving_thread created error : ");
		exit(0);
	}
	thr_id2 = pthread_create(&p_thread[1], NULL, sending_thread, NULL);
	if (thr_id2 < 0) {
		perror("sending_thread created error : ");
		exit(0);
	}

	pthread_join(p_thread[0], (void **)&status);
	pthread_join(p_thread[1], (void **)&status);

	return 0;
}


/* step 3: control servo motor behaviors */
int actuate(void) {
	char msg;

	while(TRUE) {
		if(msg=="OK") { // wait until read the signal
			break;	// do something
		}
	}

	// do something

	return 0;
}


/* before stop */
int stop(void) {
	return 0;
}


/* main func of client pi 3 */
int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	init_socket(argv[1], argv[2]);
	init_mask();


	// process_cube();
	
	task();
	
	// actuate();

	// stop();
	close(sock);

	return 0;
}
