#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFMAX 1000

int i = 0;

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

	long long tx = 0;
	long long rx[2];

	while (1) {
	        write(clnt_sock, &tx, sizeof(long long));
		tx++;
		usleep(1000000);
		
		read(clnt_sock, &rx, 2 * sizeof(long long));
		usleep(1000000);

		fprintf(stdout, "parent: %lld \n current: %lld \n\n", rx[0], rx[1]);
		usleep(1000000);
	}

	close(clnt_sock);
	close(serv_sock);

	return (0);
}

