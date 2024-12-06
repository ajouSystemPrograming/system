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
static int GPIODirection(int pin, int dir) {#include <arpa/inet.h>

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
// 밝기에 따라 led를 점등시키는 함수 (성공 시 0 반환)
int led_breathing() {
	int fd = open(DEVICE, O_RDWR);
	if (fd <= 0) {
    	perror("Device open error");
		return -1;
	}

	if (prepare(fd) == -1) {
    	perror("Device prepare error");
		return -1;
	}

	if (GPIOExport(POUT) == -1) {
		printf("gpio export err\n");
    	return 1;
	}

	if (GPIODirection(POUT, OUT) == -1) {
		printf("gpio direction err\n");
    	return 2;
	}

	int t0;
	while (1) {
		printf("value: %d\n", t0=readadc(fd, 0));
    if(t0 < PROPER_BRIGHTNESS) // t0은 0 부터 1023 사이 값
	    GPIOWrite(POUT, 0);
		pthread_exit(NULL);
    else
	    GPIOWrite(POUT, 1);
	    usleep(10000);
	}
	
	if (GPIOUnexport(POUT) == -1) {
    	return 4;
	}
}
// 메인 함수
int main(int argc, char *argv[]) {
	mask[0]=1;
	for(int i = 1; i < 24; i++)
		mask[i] = mask[i-1]*6;
		
	// 테스트 조도센서
	if (0 == led_breathing()) {
		printf("led_breathing completed.\n");
	}

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
	
	// 조도센서 값 전달
	//if (0 == led_breathing()) {
	//	printf("led_breathing completed.\n");
	//}
	
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
