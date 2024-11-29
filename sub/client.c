#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define REPEAT 10

// task management buffer size
#define INBUF_MAX 10000
#define OUTBUF_MAX 30000

// task buffers (queue)
long long inbuf[INBUF_MAX];

long long outbuf_parents[OUTBUF_MAX];
long long outbuf[OUTBUF_MAX];

int in_head = 0, in_tail = 0;
int outp_head = 0, outp_tail = 0;
int out_head = 0, out_tail = 0;

int fin = 0;

int sock;
struct sockaddr_in serv_addr;

static long long mask[24];

void init_mask(void) {
	mask[0] = 1;
	for(int i0 = 1; i0 < 24; i0++)
		mask[i0] = mask[i0-1]*6;
}


int queue_is_empty(long long *buffer, int *head, int *tail) {
	if(*head == *tail) {
		return 1;
	}

	return 0;
}

int queue_is_full(long long *buffer, int *tail, int size) {
	if(*tail >= size) {
		return 1;
	}

	return 0;
}

void enqueue(long long task, long long *buffer, int *head, int *tail, int size) {
	if(queue_is_full(buffer, tail, size)) {
		fprintf(stderr, "queue is full!\n");

		return;
	}

	buffer[*tail] = task;
	(*tail)++;

	return;
}

long long dequeue(long long *buffer, int *head, int *tail, int size) {
	long long ret;

	if(queue_is_empty(buffer, head, tail)) {
		fprintf(stderr, "queue is empty!\n");

		return -1;
	}

	ret = buffer[*head];
	(*head)++;

	return ret;
}


void *pull(void *arg) {
	long long task;

	while(1) {
		if(queue_is_full(inbuf, &in_tail, INBUF_MAX)) {
			fprintf(stdout, "full in queue: waiting...\n");
		}
		else {
			task = read(sock, &task, sizeof(long long));
			enqueue(task, inbuf, &in_head, &in_tail, INBUF_MAX);
			fprintf(stdout, "pulling from server: %lld \n", task);
		}

		usleep(1000000);
	}

	return NULL;
}

void *push(void *arg) {
	long long arr[2];

	while(1) {
		if(queue_is_empty(outbuf_parents, &outp_head, &outp_tail) || queue_is_empty(outbuf, &out_head, &out_tail)) {
			fprintf(stdout, "empty out queue: waiting...\n");
		}
		else {
			arr[0] = dequeue(outbuf_parents, &outp_head, &outp_tail, OUTBUF_MAX);
			arr[1] = dequeue(outbuf, &out_head, &out_tail, OUTBUF_MAX);
			write(sock, &arr, 2 * sizeof(long long));
			fprintf(stdout, "pushing to server: %lld, %lld \n", arr[0], arr[1]);
		}

		usleep(1000000);
	}

	return NULL;
}

/*
   void *calculate(void *arg) {
   while(1) {
   if(queue_is_empty(inbuf, &inbuf_head, &inbuf_tail) || queue_is_full(outbuf, &outbuf_tail, OUTBUF_MAX)) {
   fprintf(stdout, "queue not ready: waiting...\n");
   }
   else {
   fprintf(stdout, "do 1->3 processing\n");
   }

   usleep(1000);
   }

   return NULL;
   }
   */

long long X(long long l0){
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

long long Y(long long l0){
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

long long Z(long long l0){
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

void *calculate(void *arg) {
	long long l0;

	while(!fin) {
		if(!queue_is_empty(inbuf, &in_head, &in_tail)) {
			l0 = dequeue(inbuf, &in_head, &in_tail, INBUF_MAX);

			enqueue(l0, outbuf_parents, &outp_head, &outp_tail, OUTBUF_MAX);
			enqueue(X(l0), outbuf, &out_head, &out_tail, OUTBUF_MAX);

			enqueue(l0, outbuf_parents, &outp_head, &outp_tail, OUTBUF_MAX);
			enqueue(Y(l0), outbuf, &out_head, &out_tail, OUTBUF_MAX);

			enqueue(l0, outbuf_parents, &outp_head, &outp_tail, OUTBUF_MAX);
			enqueue(Z(l0), outbuf, &out_head, &out_tail, OUTBUF_MAX);
			usleep(1000000);
		}
		else {
			fprintf(stdout, "queue not ready: waiting...\n");
		}
	}

	fprintf(stdout, "calculation fin\n");

	return NULL;
}

void error_handling(char *message) {
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}

/*
   int main(void) {
   init_mask();

   pthread_t p_thread[2];
   int thr_id;
   int status;
   char p1[] = "puller";
   char p2[] = "pusher";
   char pM[] = "calculator";

   thr_id = pthread_create(&p_thread[0], NULL, pull, (void *)p1);
   if (thr_id < 0) {
   perror("thread create error : ");
   exit(0);
   }

   thr_id = pthread_create(&p_thread[1], NULL, push, (void *)p2);
   if (thr_id < 0) {
   perror("thread create error : ");
   exit(0);
   }

   calculate((void *)pM);
   pthread_join(p_thread[0], (void **)&status);
   pthread_join(p_thread[1], (void **)&status);

   return 0;
   }
   */


int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1) error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");

	printf("Connection established\n");

	init_mask();

	pthread_t p_thread[2];
	int thr_id;
	int status;
	char p1[] = "puller";
	char p2[] = "pusher";
	char pM[] = "calculator";

	thr_id = pthread_create(&p_thread[0], NULL, pull, (void *)p1);
	if (thr_id < 0) {
		perror("thread create error : ");
		exit(0);
	}

	thr_id = pthread_create(&p_thread[1], NULL, push, (void *)p2);
	if (thr_id < 0) {
		perror("thread create error : ");
		exit(0);
	}

	calculate((void *)pM);
	pthread_join(p_thread[0], (void **)&status);
	pthread_join(p_thread[1], (void **)&status);

	close(sock);

	return 0;
}

