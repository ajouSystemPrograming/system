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
#define MAX_BUFFER 5000000
#define fin 0
int temp;

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
//long long msg1; // receive 1
//long long msg2[3]; // send 1
long long msg1; // receive many
long long msg2[3]; // send many

/*
long long temp_input[100] = {
		473911131457577840,
		123911131457567840,
		212391113145756784,
		512391113111314572,
		612391113111314573,
		712391113111314574,
		912391113111314575,
		812391113411314573,
		112391113561131457,
		212391113711314573,
}; // 테스트용
*/
//int prev_state = 1; // 버튼 사용 시1
//int state = 1; // 버튼 사용 시2



// 인풋 버퍼와 아웃풋 버퍼
long long inputBuffer[MAX_BUFFER];
long long outputBuffer[MAX_BUFFER][3];
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
			pthread_exit(NULL);
		}
		

		usleep(2000000);
	}

	// Disable GPIO pins
	if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN) || 1 == GPIOUnexport(POUT2)) return (4);

	printf("complete\n");
}
//큐브를 돌린 결과를 반환해주는 함수
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
// 인풋 버퍼에서 전개도 하나를 꺼내와, 돌린 결과 아웃풋 3개를 버퍼에 넣는 함수

void cal() {
    long long l0;
	int t0 = inputHead++;
	l0 = inputBuffer[t0];
	outputBuffer[outputTail][0] = l0;
	outputBuffer[outputTail][1] = X(l0);
	outputBuffer[outputTail][2] = 0;
	outputTail++;
	//usleep(500 * 100);
	//printf("[l0 :%lld, X(l0) : %lld, Y(l0) : %lld, Z(l0) : %lld]\n", l0, outputBuffer[outputTail-1][1], Y(l0), Z(l0));
	outputBuffer[outputTail][0] = l0;
	outputBuffer[outputTail][1] = Y(l0);
	outputBuffer[outputTail][2] = 1;
	outputTail++;
	//usleep(500 * 100);
	
	outputBuffer[outputTail][0] = l0;
	outputBuffer[outputTail][1] = Z(l0);
	outputBuffer[outputTail][2] = 2;
	outputTail++;
	
	//usleep(500 * 100);
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
	//
	printf("receiving thread...\n");
	
	int k = 0;
	//
	int test_count1;
	while(1){
			// 1.
		if (-1 == read(sock, &msg1, sizeof(msg1)))
			error_handling("msg1 read() error");
		k=0;
		//printf("%lld\n", msg1);
		if (-1 == msg1)
			pthread_exit(NULL);
		if (msg1) { // 받은 값이 초기값인 0 이라면 실행하지 않음.
			printf("[Receive message from Server : %lld]\n",  msg1); 
			// 2.
			inputBuffer[inputTail] = msg1;
			inputTail++;
			cal();
			printf("[current inputHead and inputTail : %d, %d]\n\n", inputHead, inputTail);
		} else {
			//printf("Input buffer is Empty!\n");
		}
	}
	

}

/*
1. 소켓 통신으로 아웃풋 버퍼에 있는 msg2를
->  msg2 = outputBuffer[outputTail][0];
    msg3 = outputBuffer[outputTail++][1];
2. 메인 파이로 전송하는 함수
-> write 사용
*/
void *sending_thread(void *data) {
	printf("sending thread...\n");
	int test_count2 = 0;
	while (1) {
		if (outputHead < outputTail) { // 아웃풋 버퍼가 비어있지 않을 때만 실행
			// 1.
			int t0 = outputHead++;
			msg2[0] = outputBuffer[t0][0];
			msg2[1] = outputBuffer[t0][1];
			msg2[2] = outputBuffer[t0][2];
			printf("[current outputHead and outputTail : %d, %d], %lld, %lld, %lld\n", outputHead, outputTail, msg2[0], msg2[1], msg2[2]);
			// 2.
			//if (-1 == write(sock, msg2, sizeof(msg2)))
			//	error_handling("msg2 write() error");
			//write(sock, &msg2[0], sizeof(msg2[0]));
			//write(sock, &msg2[1], sizeof(msg2[1]));
			//write(sock, &msg2[2], sizeof(msg2[2]));

			write(sock, msg2, sizeof(msg2));
			//usleep(1);
			//temp = 1;
			usleep(1);
			/*
			inputBuffer[inputTail] = msg2[1];
			printf("inputTail : %d\n", inputTail);
			inputTail++;
			*/
			
			
			printf("Sending message to Server : %lld, %lld\n\n", msg2[0], msg2[1]);
		} else {
			//printf("Output buffer is Empty!\n");
		}
		//
		//test_count2++;
		//if (test_count2 > 10000)
		//	break;
			//
		//usleep(500 * 100);

	}
}

int main(int argc, char *argv[]) {
	mask[0]=1;
	for(int i = 1; i < 24; i++)
		mask[i] = mask[i-1]*6;
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
	//msg2[0]=1000;
	//write(sock, msg2, sizeof(msg2));	
	
	// 이제 쓰레드 실행을 위한 코드들
	pthread_t p_thread[3];
	int thr_id;
	int thr_id2;
	int thr_id3;
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
	/*
	 * thr_id3 = pthread_create(&p_thread[2], NULL, cal, NULL);
	if (thr_id3 < 0) {
		perror("reveiving_thread created error : ");
		exit(0);
	}
	*/
	//cal();
	pthread_join(p_thread[0], (void **)&status);
	pthread_join(p_thread[1], (void **)&status);
	//pthread_join(p_thread[2], (void **)&status);

	close(sock);


	return (0);
}
