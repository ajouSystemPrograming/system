#ifndef __CUBE_SOLVING_H__
#define __CUBE_SOLVING_H__

int fin = 0;
int temp;

#define SIZE 10
long long rec_buf[SIZE];
long long buffer[500000][SIZE];
int head;
int tail;

// socket variables
int sock;
struct sockaddr_in serv_addr;

/* error handler */
void error_handling(char *message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(-1);
}

/* step 0: init */
int init_socket(char *argv_1, char *argv_2) {
	// Communication settings
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1) error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv_1);
	serv_addr.sin_port = htons(atoi(argv_2));

	while(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1);

	printf("Connection established\n");

	return 0;
}

void *receiving_thread(void *data) {
	//
	printf("receiving thread...\n");
	
	int k = 0;
	//
	int test_count1;
	while(!fin){
			// 1. 5000 x 1000 
		int t1 = read(sock, buffer[tail], sizeof(buffer[tail]));
		if (t1 == 0)
			exit(0);
		if (-1 == t1) 
			error_handling("msg1 read() error");
		if (-2 == buffer[tail][0]) // can't solve
			exit(0);
		if (-1 == buffer[tail][0]) {
			printf("-1 is received\n");
			fin = 1;
			pthread_exit(NULL);
		}
		
		k=0;
		tail++;
		
	}
}

void *sending_thread(void *data) {
	printf("sending thread...\n");
	int test_count2 = 0;
	while (!fin) {
		if (head < tail) { 
			
			long long msg[SIZE*4]; // original data 
			long long l0;
			for(int i = 0; i < SIZE; i++)
			{
				l0 = buffer[head][i];
				if(l0==0)
				{
					printf("0->%d %d\n",head,i);
						break;
				}
				msg[i*4] = l0;
				msg[i*4+1] = X(l0);
				msg[i*4+2] = Y(l0);
				msg[i*4+3] = Z(l0);
				
			}
			
			head++;
			
			int t0 = 0;
			t0 = write(sock, msg, sizeof(msg));
			test_count2 = 0;
			
		} else {
	
		}
	
	}
}

#endif
