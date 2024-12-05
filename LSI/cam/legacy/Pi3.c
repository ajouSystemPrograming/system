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

#define TRUE 1
#define FALSE 0

#define BUFMAX 100

// socket variables
int sock;
struct sockaddr_in serv_addr;

// buffer array
char buffer[BUFMAX];

char *msg;

/* error handler */
void error_handling(char *message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


/* step 0: init */
int init(char *argv_1, char *argv_2) {
	// Communication settings
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1) error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv_1);
	serv_addr.sin_port = htons(atoi(argv_2));

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");

	printf("Connection established\n");

	return 0;
}


/* step 1: camera sensing & processing */
int process_cube(void) {
	char *msg;
	for(pl=0; pl<6; pl++) { // for 6 planes of the cube
		while(TRUE) {
			if(msg=="OK") { // wait until read the signal
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


/* step 2: */
int task(void) {
	while(TRUE) {
		if(msg=="OK") { // wait until read the signal
			// do something
		}
	}

	return 0;
}


/* step 3: control servo motor behaviors */
int actuate(void) {
	while(TRUE) {
		if(msg=="OK") { // wait until read the signal
			// do something
		}
	}

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

	init(argv[1], argv[2]);

	// read/write to initialize connection
	write(sock, &buffer, sizeof(char));
	read(sock, &buffer, sizeof(char));


	close(sock);

	return 0;
}
