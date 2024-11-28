#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#define TOTALFORK 5

int main() {
	pid_t pids[TOTALFORK];
	int runProcess = 0;
	int state;
	while(runProcess < TOTALFORK) {
		pids[runProcess] = fork();
		if(pids[runProcess] < 0) {
			return -1;
		} else if(pids[runProcess] == 0) {//child process
			printf("child process %ld is running.\n", (long)getpid());
			sleep(1);
			printf("%d process is terminated.\n", getpid());
			exit(0);
		} else { //parent process
			//wait(&state);
			printf("parent %ld, child %d\n", (long)getpid(), pids[runProcess]);
		}
		runProcess++;
	}
	
	printf("parent process is terminated\n");
	return 0;
}
