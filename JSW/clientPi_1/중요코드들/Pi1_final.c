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
#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define POUT 23 // 초음파센서
#define PIN 24 // 초음파센서
#define PWM 0
#define SIZE 10 // sending and receiving buffer size

int temp;
int sock;
long long buffer[5000000/SIZE][SIZE];
int head;
int tail;
static long long mask[24];

//에러 함수
void error_handling(char *message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
//GPIO 연결 함수들 
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
//PWM 관련 함수들
static int PWMExport(int pwmnum) {

  char buffer[BUFFER_MAX];
  int fd, byte;

  // TODO: Enter the export path.
  fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open export for export!\n");
    return (-1);
  }

  byte = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
  write(fd, buffer, byte);
  close(fd);

  sleep(1);

  return (0);
}
static int PWMEnable(int pwmnum) {
  static const char s_enable_str[] = "1";

  char path[DIRECTION_MAX];
  int fd;

  // TODO: Enter the enable path.
  snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm0/enable", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in enable!\n");
    return -1;
  }

  write(fd, s_enable_str, strlen(s_enable_str));
  close(fd);

  return (0);
}
static int PWMWritePeriod(int pwmnum, int value) {
  char s_value_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  // TODO: Enter the period path.
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/period", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in period!\n");
    return (-1);
  }
  byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_value_str, byte)) {
    fprintf(stderr, "Failed to write value in period!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return (0);
}
static int PWMWriteDutyCycle(int pwmnum, int value) {
  char s_value_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  // TODO: Enter the duty_cycle path.
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm0/duty_cycle", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open in duty cycle!\n");
    return (-1);
  }
  byte = snprintf(s_value_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_value_str, byte)) {
    fprintf(stderr, "Failed to write value in duty cycle!\n");
    close(fd);
    return -1;
  }
  close(fd);

  return (0);
}
//큐브를 각 방향으로 돌렸을 때의 결과를 반환해주는 함수 (X, Y, Z가 방향을 의미)
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
// SIZE만큼 개수의 전개도를 서버로부터 받아 입력 버퍼에 넣는 함수
void *receiving_thread(void *data) {
	//
	printf("receiving thread...\n");
	
	int k = 0;
	//
	int test_count1;
	while(1){
			// 1. 5000 x 1000 
		int t1 = read(sock, buffer[tail], sizeof(buffer[tail]));
		if (-1 == t1) 
			error_handling("buffer read() error\n");
		//printf("t1 : %d\n", t1);
		if (-1 == buffer[tail][0]) {
			printf("-1 is received\n");
			exit(0);
		}
		
		k=0;
		tail++;
		//printf("tail : %d\n", tail);
		
	}
}
// 버퍼에서 값을 꺼내 (원본, X회전결과, Y회전결과, Z회전결과)로 4개로 나눠진 값을 출력 버퍼에 저장하고,
// 그렇게 SIZE*4만큼 개수의 전개도를 서버한테 전송하는 함수.
void *sending_thread(void *data) {
	printf("sending thread...\n");
	int test_count2 = 0;
	while (1) {
		if (head < tail) { 
			
			long long msg[SIZE*4]; // 원본 
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
			//printf("%d\n", t0);
			test_count2 = 0;
			
			//printf("head : %d\n\n", head);
		} else {
	
		}
	
	}
}
// 거리에 따라 led를 점등시키는 함수
int led_breathing() {
	// Enable GPIO pins
	if (-1 == GPIOExport(POUT) || -1 == GPIOExport(PIN)) {
		printf("gpio export err\n");
		return (1);
	}
	// wait for writing to export file
	usleep(100000);
	// Set GPIO directions
	if (-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(PIN, IN)) {
		printf("gpio direction err\n");
		return (2);
	}
	if (-1 == PWMExport(PWM)) {
		printf("PWM export err\n");
		return (4);
	}
	// wait for writing to export file
	usleep(100000);
	if (-1 == PWMWritePeriod(PWM, 10000000)) {
		printf("PWM period err\n");
		return (5);
	}
	usleep(10000);
	if (-1 == PWMWriteDutyCycle(PWM, 0)) {
		printf("PWM duty cycle err\n");
		return (6);
	}
	if (-1 == PWMEnable(PWM)) {
		printf("PWM duty cycle err\n");
		return (7);
	}
	// init ultrawave trigger
	GPIOWrite(POUT, 0);
	usleep(10000);
	// start
	clock_t start_t, end_t;
	double time;
	double distance = 0;
	int one = 1; // 1을 보내기 위한 flag
	while(1) {
		int isPic = 0; // 카메라 파이에서 사진을 찍었는지 여부를 확인하기 위한 숫자.
		
		if (-1 == read(sock, &isPic, sizeof(isPic)))
			error_handling("pic read() error");
		if ((isPic & 0x03) == 0x01) {  // 하위 2비트가 01인지 확인
			if (isPic & 0x80) {  // 7번 비트가 1인지 확인
				printf("Picture is captured. Stopping sensoring distance.\n");
				return 0;
			}
		}
		// isPic의 0, 1번 비트가 01(10진수로 1)일 때만 받아들이고(아니면 무시)
		// 이 때, 7번 비트가 1이면 다음단계(즉, return 0;)
		
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
		if (-1 == write(sock, &distance, sizeof(distance)))
			error_handling("distance write() error");
		
		// '가장 최적의 거리'일 때를 20~30cm이라 하자.
		if (distance < 0) { // ~ 0
			printf("@@Sensor err! Distance should be positive\n");
			PWMWriteDutyCycle(PWM, 00000);
			
		} else if (distance >= 50.00) { // 50 ~ 
			printf("@@Please put it clooooser\n");
			PWMWriteDutyCycle(PWM, 00000);

		} else if (distance < 20) { // 0 ~ 20
			printf("@@Please put it farther\n");
			PWMWriteDutyCycle(PWM, distance * 150000);
			
		} else if (distance < 30){ // 20 ~ 30
			printf("@@Proper distance!!\n");
			PWMWriteDutyCycle(PWM, 10000000);
			if (-1 == write(sock, &one, sizeof(one)))
				error_handling("Server is not receiving your brightness.\n");
			
		} else if (distance >= 30.00) { // 30 ~ 50
			printf("@@Please put it closer\n");
			PWMWriteDutyCycle(PWM, (50 - distance) * 150000);
		} 
		
		usleep(1000000);
	}

	// Disable GPIO pins
	if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN)) return (4);

	printf("complete\n");
}
// 메인 함수
int main(int argc, char *argv[]) {
	mask[0]=1;
	for(int i = 1; i < 24; i++)
		mask[i] = mask[i-1]*6;
	
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
	
	// 초음파센서 값 전달
	if (0 == led_breathing()) {
		printf("led_breathing completed.\n");
	}
	
	// 이제 쓰레드 실행을 위한 코드들
	pthread_t p_thread[2];
	int thr_id;
	int thr_id2;
	int status;
	char p1[] = "receiving_thread";
	char p2[] = "sending_thread";

	// 쓰레드 예외처리
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

	close(sock);


	return (0);
}
