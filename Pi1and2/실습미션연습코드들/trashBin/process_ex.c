#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void main(void)
{
	pid_t pid;
	pid = fork();   		//make child process
	if(pid == -1) {
		printf("can't fork\n");
		exit(0);
	}
	if(pid == 0) {
		printf("child process id: %d\n", getpid());
		sleep(1);
		exit(0);
	}

	else {
		printf("parent process id: %d\n", getpid());
		sleep(1);
		exit(1);
	}
	return;
}
