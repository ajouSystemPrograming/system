#include <arpa/inet.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256
#define MAX_BUFFER 5000000
#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define POUT 17 // LED OUT
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define PROPER_BRIGHTNESS 500 // 조도센서 적정 밝기 기준값
#define SIZE 10 // sending and receiving buffer size

#define PWM 0

#define MOTOR_0 1450000
#define MOTOR_90 400000

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

	char path[DIRECTION_MAX];
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
	}

	close(fd);
	return (0);
}
//조도 센서 관련 변수 및 함수들
static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = 0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;
static int prepare(int fd) {
	if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1) {
		perror("Can't set MODE");
		return -1;
	}

	if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1) {
		perror("Can't set number of BITS");
		return -1;
	}

	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1) {
		perror("Can't set write CLOCK");
		return -1;
	}

	if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1) {
		perror("Can't set read CLOCK");
		return -1;
	}

	return 0;
}
uint8_t control_bits_differential(uint8_t channel) {
	return (channel & 7) << 4;
}
uint8_t control_bits(uint8_t channel) {
	return 0x8 | control_bits_differential(channel);
}
int readadc(int fd, uint8_t channel) {
	uint8_t tx[] = {1, control_bits(channel), 0};
	uint8_t rx[3];

	struct spi_ioc_transfer tr = {
			.tx_buf = (unsigned long)tx,
			.rx_buf = (unsigned long)rx,
			.len = ARRAY_SIZE(tx),
			.delay_usecs = DELAY,
			.speed_hz = CLOCK,
			.bits_per_word = BITS,
	};

	if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {
		perror("IO Error");
		abort();
	}

	return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}
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
   PWMWriteDutyCycle(servo, MOTOR_0);
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
		if (-2 == buffer[tail][0]) // can't solve
			exit(0);
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
// 밝기에 따라 led를 점등시키는 함수 (성공 시 0 반환)
int led_breathing() {
	int fd = open(DEVICE, O_RDWR);
	if (fd <= 0) {
    	perror("Device open error");
    	exit(-1);
	}

	if (prepare(fd) == -1) {
    	perror("Device prepare error");
    	exit(-1);
	}

	if (GPIOExport(POUT) == -1) {
		printf("gpio export err\n");
    	exit(-1);
	}
	
	usleep(100000);


	if (GPIODirection(POUT, OUT) == -1) {
		printf("gpio direction err\n");
    	exit(-1);
	}
	
	GPIOWrite(POUT, 0);
	usleep(10000);

	// start
	int brightness;
	int prev_state = 0;
	int state = 0;
	int flag_pi3 = 3; // 'Pi2' 이라는 걸 알리기 위한 flag
	int one = 3;
	while (1) {

		int isPic = 0; // 카메라 파이에서 사진을 찍었는지 여부를 확인하기 위한 숫자.
		
		int t0 = read(sock, &isPic, sizeof(isPic));
		printf("t0 : %d\n", t0);
		printf("isPic : %d\n", isPic);
		if(t0 == 0) exit(0);
		if(t0 > 0) { // 뭔가 받았을 때
			if (isPic < 0) {  // 다음 단계로 넘어가라는 표식
			printf("Picture is captured. Stopping sensoring distance.\n");
			return 0;
			}
			else  { // -1 아니면 내 꺼 아님.
				printf("It's not for terminating!!\n");		
				continue;
			}
		}
		
		printf("value: %d\n", brightness = readadc(fd, 0));
		printf("Current brightness : %d\n", brightness);
		if (brightness < PROPER_BRIGHTNESS) { // t0은 0 부터 1023 사이 값
			printf("@@Set it brighter!!\n");
			GPIOWrite(POUT, 0);
			state = 0;
			if (1 == prev_state) {
				printf("state is changed\n");
				one = 3; // 밝았다가 어두워졌다고 서버에게 전달. 2번째 비트를 0으로.
				if (-1 == write(sock, &flag_pi3, sizeof(flag_pi3))) // Pi3 임을 알리기 위한 write
					error_handling("Server is not receiving your flag_pi2.\n");
				printf("flag_pi3 : %d\n", flag_pi3);
				if (-1 == write(sock, &one, sizeof(one))) // 적정 밝기임을 알리기 위한 write
					error_handling("Server is not receiving your brightness.\n");
				printf("one : %d\n", one);
			}
		} else {
			printf("@@Proper brightness!!\n");
			GPIOWrite(POUT, 1);
			state = 1;
			if (0 == prev_state) {
				printf("state is changed\n");
				one = 7; // 어두웠다가 밝아졌다고 서버에게 전달. 2번째 비트를 1로.
				if (-1 == write(sock, &flag_pi3, sizeof(flag_pi3))) // Pi3 임을 알리기 위한 write
					error_handling("Server is not receiving your flag_pi2.\n");
				printf("flag_pi3 : %d\n", flag_pi3);
				if (-1 == write(sock, &one, sizeof(one))) // 적정 밝기임을 알리기 위한 write
					error_handling("Server is not receiving your brightness.\n");
				printf("one : %d\n", one);
			} //else {
				//flag_pi3 = 0; // 내 메시지를 받지 말라고 서버에게 전달. 2번째 비트를 0으로.
				//if (-1 == write(sock, &flag_pi2, sizeof(flag_pi2))) // Pi2 임을 알리기 위한 write
				//error_handling("Server is not receiving your flag_pi2.\n");
			//}
			
		
		}
		prev_state = state;
		usleep(1000000);
	}

	// Disable GPIO pins
	if (GPIOUnexport(POUT) == -1) {
    	exit(0);
	}
	printf("complete\n");

}
int actuate(void) {
	int msg;
	int sig = 3; // identifier

	
	while(1) {
		read(sock,&msg,sizeof(msg));
		printf("spin %d\n", msg);
		if(msg==-1) // if step 3 finished
			break;
		if(msg==3) { // if the command is for mine
			printf("spin is starting.\n");
			spin(PWM, MOTOR_90);
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
		
		  // PWM1 init
	PWMExport(PWM);
	PWMWritePeriod(PWM, 10000000);
	PWMWriteDutyCycle(PWM, MOTOR_0);
	PWMEnable(PWM);

	// 테스트 조도센서
	//if (0 == led_breathing()) {
	//	printf("led_breathing completed.\n");
	//}

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

	while(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1);
	//	error_handling("connect() error");
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
	// 조도 센서값 읽는 건 논 블로킹 모드로 실행할 거임.
		
	// 조도 센서 값 전달
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
