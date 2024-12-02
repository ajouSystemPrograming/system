#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_MAX 1000000

long long tasks[BUF_MAX];
int head = 0, tail = 0;

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

void print_queue(int *head, int *tail) {
        printf("head: %d, tail: %d\n", *head, *tail);
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


void error_handling(char *message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[]) {
	int serv_sock, clnt_sock = -1;
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_size;

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_sock == -1) error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error");

	if (listen(serv_sock, 5) == -1) error_handling("listen() error");

	if (clnt_sock < 0) {
		clnt_addr_size = sizeof(clnt_addr);
		clnt_sock =
				accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
		if (clnt_sock == -1) error_handling("accept() error");
	}

	printf("Connection established\n");

	long long tx;
	long long rx[2];
	
	long long init = 4271069981394162510;
	enqueue(init, tasks, &head, &tail, BUF_MAX);
	print_queue(&head, &tail);
	printf("\n\n");

	while (1) {
		if(!queue_is_empty(tasks, &head, &tail)) {
			tx = dequeue(tasks, &head, &tail, BUF_MAX); 
			write(clnt_sock, &tx, sizeof(long long));
			printf("Server Sending: %lld", tx);
			print_queue(&head, &tail);
			usleep(300000);
		}

		if(!queue_is_full(tasks, &tail, BUF_MAX)) {
			read(clnt_sock, &rx, 2 * sizeof(long long));
			enqueue(rx[1], tasks, &head, &tail, BUF_MAX);
			printf("Server Receiving: \n");
			print_queue(&head, &tail);
			fprintf(stdout, "parent: %lld \n current: %lld \n\n", rx[0], rx[1]);
			usleep(300000);
		}
	}

	close(clnt_sock);
	close(serv_sock);

	return (0);
}

