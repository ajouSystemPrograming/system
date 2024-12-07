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
#define POUT2 17
#define SIZE 10 // sending and receiving buffer size

int temp;
int sock;
long long buffer[5000000/SIZE][SIZE];
int head;
int tail;
static long long mask[24];
int fin = 0;

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
  char path[VALUE_MAX];
  // TODO: Enter the export path.
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/export", pwmnum);
  fd = open(path, O_WRONLY);
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
  snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable",pwmnum);
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
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/period",pwmnum);
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
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle",pwmnum);
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
static int PWMUnexport(int pwmnum) {
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;
  char path[VALUE_MAX];
  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/unexport", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}
static int PWMDisable(int pwmnum) {
   static const char s_enable_str[] = "0";

   char path[DIRECTION_MAX];
   int fd;

   snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
   fd = open(path, O_WRONLY);
   if (-1 == fd) {
      fprintf(stderr, "Failed to open in enable!\n");
      return -1;
   }

   write(fd, s_enable_str, strlen(s_enable_str));
   close(fd);

   return (0);
}
int spin(int servo, int val) {
   // PWMEnable(servo);
   PWMWriteDutyCycle(servo, val);
   // PWMDisable(servo);
   usleep(1000000);
   PWMWriteDutyCycle(servo, 400000);
   usleep(1000000);
   return 0;
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
			fin = 1;
			pthread_exit(NULL);
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
	while (!fin) {
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
	pthread_exit(NULL);
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
	
	// wait for writing to export file
	usleep(100000);
	
	// init ultrawave trigger
	GPIOWrite(POUT, 0);
	GPIOWrite(POUT2, 0);
	usleep(10000);
	// start
	clock_t start_t, end_t;
	double time;
	double distance = 0;
	int prev_state = 0;
	int state = 0;
	int flag_pi1 = 1; // 'Pi1' 이라는 걸 알리기 위한 flag
	int one = 1;
	while(1) {
		
		
		int isPic = 0; // 카메라 파이에서 사진을 찍었는지 여부를 확인하기 위한 숫자.
		
		int t0 = read(sock, &isPic, sizeof(isPic));
		printf("t0 : %d\n", t0);
		printf("isPic : %d\n", isPic);
		
		if(t0 > 0) { // 뭔가 받았을 때
			if (isPic < 0) {  // 다음 단계로 넘어가라는 표식
			printf("Picture is captured. Stopping sensoring distance.\n");
			return 0;
			}
			else { // -1 아니면 내 꺼 아님.
				printf("It's not for terminating!!\n");
				continue;
			}
		}
		
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
		flag_pi1 = 1;
		printf("distance : %.2lfcm\n", distance);
		
		// '가장 최적의 거리'일 때를 20~30cm이라 하자.
		if (distance < 0) { // ~ 0
			printf("@@Sensor err! Distance should be positive\n");
			GPIOWrite(POUT2, 0);
			state = 0;
			if (1 == prev_state && 0 == state) {
				printf("state is changed\n");
				one = 1; // 내 메세지를 받아 달라고 서버에게 전달. 2번째 비트를 1로.
				if (-1 == write(sock, &flag_pi1, sizeof(flag_pi1))) // Pi1 임을 알리기 위한 write
					error_handling("Server is not receiving your flag_pi1.\n");
				printf("flag_pi1 : %d\n", flag_pi1);
				if (-1 == write(sock, &one, sizeof(one))) // 적정 거리임을 알리기 위한 write
					error_handling("Server is not receiving your distance.\n");
				printf("one : %d\n", one);
				
			}
		} else if (distance < 20) { // 0 ~ 20
			printf("@@Please put it farther\n");
			GPIOWrite(POUT2, 0);
			state = 0;
			if (1 == prev_state && 0 == state) {
				printf("state is changed\n");
				one = 1; // 내 메세지를 받아 달라고 서버에게 전달. 2번째 비트를 1로.
				if (-1 == write(sock, &flag_pi1, sizeof(flag_pi1))) // Pi1 임을 알리기 위한 write
					error_handling("Server is not receiving your flag_pi1.\n");
				printf("flag_pi1 : %d\n", flag_pi1);
				if (-1 == write(sock, &one, sizeof(one))) // 적정 거리임을 알리기 위한 write
					error_handling("Server is not receiving your distance.\n");
				printf("one : %d\n", one);
			}
		} else if (distance < 30){ // 20 ~ 30
			printf("@@Proper distance!!\n");
			GPIOWrite(POUT2, 1);
			state = 1;
			if (0 == prev_state && 1 == state) {
				printf("state is changed\n");
				one = 5; // 내 메세지를 받아 달라고 서버에게 전달. 2번째 비트를 1로.
				if (-1 == write(sock, &flag_pi1, sizeof(flag_pi1))) // Pi1 임을 알리기 위한 write
					error_handling("Server is not receiving your flag_pi1.\n");
				printf("flag_pi1 : %d\n", flag_pi1);
				if (-1 == write(sock, &one, sizeof(one))) // 적정 거리임을 알리기 위한 write
					error_handling("Server is not receiving your distance.\n");
				printf("one : %d\n", one);
			} //else {
				//flag_pi1 = 1; // 내 메세지를 받지 말라고 서버에게 전달. 2번째 비트를 0으로.
				//if (-1 == write(sock, &flag_pi1, sizeof(flag_pi1))) // Pi1 임을 알리기 위한 write
				//	error_handling("Server is not receiving your flag_pi1.\n");
			//}
			
		} else if (distance >= 30.00) { // 30
			printf("@@Please put it closer\n");
			GPIOWrite(POUT2, 0);
			state = 0;
			if (1 == prev_state && 0 == state) {
				printf("state is changed\n");
				one = 1; // 내 메세지를 받아 달라고 서버에게 전달. 2번째 비트를 1로.
				if (-1 == write(sock, &flag_pi1, sizeof(flag_pi1))) // Pi1 임을 알리기 위한 write
					error_handling("Server is not receiving your flag_pi1.\n");
				printf("flag_pi1 : %d\n", flag_pi1);
				if (-1 == write(sock, &one, sizeof(one))) // 적정 거리임을 알리기 위한 write
					error_handling("Server is not receiving your distance.\n");
				printf("one : %d\n", one);
			}
		} 
		prev_state = state;
		usleep(1000000);
	}

	// Disable GPIO pins
	if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN) || -1 == GPIOUnexport(POUT2)) return (4);
	printf("complete\n");
}

int actuate(void) {
	int msg;
	int sig = 1; // identifier

		  // PWM1 init
	PWMExport(PWM);
	PWMWritePeriod(PWM, 10000000);
	PWMWriteDutyCycle(PWM, 400000);
	PWMEnable(PWM);
	
	while(1) {
		read(sock,&msg,sizeof(msg));
		if(msg==-1) // if step 3 finished
			break;
		if(msg==1) { // if the command is for mine
			printf("spin is starting.\n");
			spin(PWM, 1450000);
		} else 
			printf("spin is not starting.\n");
		write(sock,&sig,sizeof(sig)); // always send signal who am I
	}

	PWMDisable(PWM);
	PWMUnexport(PWM);

	return 0;
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
	
	int label = 0;
	read(sock, &label, sizeof(label));
	switch(label) {
		case 1:
			goto STEP1;
		case 2:
			goto STEP2;
		case 3:
			goto STEP3;
	}
	
STEP1:
	printf("step 1 start.\n");
	int flag = fcntl(sock, F_GETFL); 
	fcntl(sock, F_SETFL, flag | O_NONBLOCK);
	// 초음파 센서값 읽는 건 논 블로킹 모드로 실행할 거임.
		
	// 초음파 센서 값 전달
	if (0 == led_breathing()) {
		printf("led_breathing completed.\n");
	}
	
	fcntl(sock, F_SETFL, flag &~ O_NONBLOCK);
	// 다시 블로킹 모드로 돌아옴.
	
STEP2:
	printf("step 2 start.\n");
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

STEP3:
	printf("step 3 start.\n");

	actuate();
	
	close(sock);
	
	
	return (0);
}
