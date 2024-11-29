#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256
#define MAX_BUFFER 3000000
#define fin 0

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define POUT 23 // 초음파센서
#define PIN 24 // 초음파센서
#define POUT2 16 // LED
//#define POUT 21 // 버튼
//#define PIN 20 // 버튼

// 쓰레드와 통신을 위한 변수들
int sock;
//int msg[2]; 테스트용
long long msg1; // receive
long long msg2[2]; // send

//int prev_state = 1; // 버튼 사용 시1
//int state = 1; // 버튼 사용 시2



// 인풋 버퍼와 아웃풋 버퍼
long long inputBuffer[MAX_BUFFER];
long long outputBuffer[MAX_BUFFER][2];
int inputHead;
int inputTail;
int outputHead;
int outputTail;
static long long mask[24];
//에러 핸들링 및 GPIO 연결 함수들
void error_handling(char *message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
static int GPIOExport(int pin) {
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return (-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
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
		return (-1);
	}
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return (0);
}
static int GPIODirection(int pin, int dir) {
	static const char s_directions_str[] = "in\0out";
	char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return (-1);
	}

	if (-1 ==
			write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return (-1);
	}

	close(fd);
	return (0);
}
static int GPIORead(int pin) {
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return (-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		return (-1);
	}

	close(fd);

	return (atoi(value_str));
}
static int GPIOWrite(int pin, int value) {
	
	static const char s_values_str[] = "01";
	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return (-1);
	}

	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return (-1);

		close(fd);
		return (0);
	}
}
// 거리에 따라 led를 점등시키는 함수

int led_breathing() {

	// Enable GPIO pins
	if (-1 == GPIOExport(POUT) || -1 == GPIOExport(PIN) || -1 == GPIOExport(POUT2)) {
		printf("gpio export err\n");
		return (1);
	}
	// wait for writing to export file
	usleep(100000);

	// Set GPIO directions
	if (-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(PIN, IN) || -1 == GPIODirection(POUT2, OUT)) {
		printf("gpio direction err\n");
		return (2);
	}

	// init ultrawave trigger
	GPIOWrite(POUT, 0);
	usleep(10000);
	// start
	clock_t start_t, end_t;
	double time;
	double distance = 0;

	while(1) {
		if (-1 == GPIOWrite(POUT, 1)) {
			printf("gpio write/trigger err\n");
			return (3);
		}

		// 1sec == 1000000ultra_sec, 1ms = 1000ultra_sec
		usleep(10);
		GPIOWrite(POUT, 0);

		while (GPIORead(PIN) == 0) {
			start_t = clock();
		}
		while (GPIORead(PIN) == 1) {
			end_t = clock();
		}

		time = (double)(end_t - start_t) / CLOCKS_PER_SEC;  // ms
		printf("time : %.4lf\n", time);
		distance = time / 2 * 34000;
		printf("distance : %.2lfcm\n", distance);

		/* 
		distance에 따른 led 출력 여부 결정
		거리가 5cm ~ 15cm 일 때만 led 점등
		*/
		if (distance < 5.00) {
			printf("Please put it farther\n");
			GPIOWrite(POUT2, 0);
		} else if (distance > 15.00) {
			printf("Please put it closer\n");
			GPIOWrite(POUT2, 0);
		} else {
			printf("Proper distance\n");
			GPIOWrite(POUT2, 1);
			return 0;
		}
		

		usleep(2000000);
	}

	// Disable GPIO pins
	if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN) || 1 == GPIOUnexport(POUT2)) return (4);

	printf("complete\n");
}
//큐브를 돌린 결과를 반환해주는 함수
long long X(long long l0){
    long l1 = (l0/mask[5])%6, l2 = (l0/mask[7])%6, l3, l4;
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
    long l1 = (l0/mask[0])%6, l2 = (l0/mask[1])%6, l3, l4;
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
    long l1 = (l0/mask[1])%6, l2 = (l0/mask[3])%6, l3, l4;
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
// 인풋 버퍼에서 전개도 하나를 꺼내와, 돌린 결과 아웃풋 3개를 버퍼에 넣는 함수
void *cal() {
    long long l0;
    while(!fin)
    {
        if(inputHead < inputTail)
        {
            l0 = inputBuffer[inputHead++];
            outputBuffer[outputTail][0] = l0;
            outputBuffer[outputTail++][1] = X(l0);
            outputBuffer[outputTail][0] = l0;
            outputBuffer[outputTail++][1] = Y(l0);
            outputBuffer[outputTail][0] = l0;
            outputBuffer[outputTail++][1] = Z(l0);
			usleep(500 * 100);
        }
    }
}

// 비어있으면 0을 출력
// 비어있지 않으면 1을 출력

/*
1. 소켓 통신으로 메인 파이에서 msg1를 받아
-> read 사용 
2. 인풋 버퍼에 넣는 함수
-> 그냥 inputBuffer[inputHead++] = msg1;
*/
void *receiving_thread(void *data) {
	int test_count1 = 0;
	while(1){
		
		
			// 1.
		if (-1 == read(sock, &msg1, sizeof(msg1)))
			error_handling("msg1 read() error"); 
		if (msg1) { // 받은 값이 초기값인 0 이라면 실행하지 않음.
			printf("Receive message from Server : %lld\n",  msg1); 
			// 2.
			inputBuffer[inputHead++] = msg1;
		} else {
			printf("Input buffer is Empty!\n");
		}
		
		test_count1++;
		if (test_count1 > 100)
			break;
		usleep(500 * 100);
	}
	

}
/*
void *receiving_thread(void *data) {
	while(1){
		
		if (inputHead < inputTail) { // 인풋 버퍼가 비어있지 않을 때만 실행
			// 1.
			if (-1 == read(sock, &msg1, sizeof(msg1)))
				error_handling("msg1 read() error"); 
			printf("Receive message from Server : %lld\n",  msg1); 
			usleep(500 * 1000);
			// 2.
			inputBuffer[inputHead++] = msg1;
		} else {
			printf("Input buffer is Empty!\n");
		}
	}
	

}
*/
/*
1. 소켓 통신으로 아웃풋 버퍼에 있는 msg2를
->  msg2 = outputBuffer[outputTail][0];
    msg3 = outputBuffer[outputTail++][1];
2. 메인 파이로 전송하는 함수
-> write 사용
*/
void *sending_thread(void *data) {
	int test_count2 = 0;
	while (1) {
		if (outputHead < outputTail) { // 아웃풋 버퍼가 비어있지 않을 때만 실행
			// 1.
			msg2[0] = outputBuffer[outputTail][0];
    		msg2[1] = outputBuffer[outputTail++][1];
			// 2.
			if (-1 == write(sock, msg2, sizeof(msg2)))
				error_handling("msg2 write() error");
			printf("Sending message to Server : [%lld, %lld]", msg2[0], msg2[1]);
			usleep(500 * 1000);
		} else {
			printf("Output buffer is Empty!\n");
		}
		test_count2++;
		if (test_count2 > 100)
			break;
		usleep(500 * 100);

	}
}

int main(int argc, char *argv[]) {
	//led_breathing();
	
	// 소켓 연결을 위한 코드들
	struct sockaddr_in serv_addr;
	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1) 
		error_handling("socket() error");
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	printf("Connection established\n");
	// 여기까지 됐으면, 메인 파이와 소켓 소켈 연결은 수립된 거임.

	// 이제 쓰레드 실행을 위한 코드들
	pthread_t p_thread[2];
	int thr_id;
	int thr_id2;
	int status;
	char p1[] = "receiving_thread";
	char p2[] = "sending_thread";
	char pM[] = "calculating_thread";

	// 쓰레드 예외처리
	//thr_id = pthread_create(&p_thread[0], NULL, receiving_thread, (void *)p1);
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

	cal();
	pthread_join(p_thread[0], (void **)&status);
	pthread_join(p_thread[1], (void **)&status);



	close(sock);


	return (0);
}
