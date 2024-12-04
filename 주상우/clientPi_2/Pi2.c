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
#define fin 0

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define POUT 17 // LED OUT
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define PROPER_BRIGHTNESS 500 // 조도센서 적정 밝기 기준값

// 쓰레드와 통신을 위한 변수들
int sock;
long long msg1; // receive many
long long msg2[3]; // send many

// 인풋 버퍼와 아웃풋 버퍼
long long inputBuffer[MAX_BUFFER];
long long outputBuffer[MAX_BUFFER][3];
int inputHead;
int inputTail;
int outputHead;
int outputTail;
static long long mask[24];

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
//조도 센서 관련 함수 및 변수들
static const char *DEVICE = "/dev/spidev0.0";
static uint8_t MODE = 0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;
//
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
void *cal() {
    long long l0;
    while(!fin)
    {
        if(inputHead < inputTail)
        {
			int t0 = inputHead++;
            l0 = inputBuffer[t0];
            outputBuffer[outputTail][0] = l0;
            outputBuffer[outputTail][1] = X(l0);
            outputBuffer[outputTail][2] = 0;
            outputTail++;
			//usleep(500 * 100);
            printf("[l0 :%lld, X(l0) : %lld, Y(l0) : %lld, Z(l0) : %lld]\n", l0, outputBuffer[outputTail-1][1], Y(l0), Z(l0));
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
	//
	printf("receiving thread...\n");
	
	int k = 0;
	//
	int test_count1;
	while(!fin){
			// 1.
		if (-1 == read(sock, buffer[tail], sizeof(buffer[0])))
			error_handling("msg1 read() error");
			if(buffer[tail][0]==-1)
			{
				fin = 1;
				//exit
			}
		k=0;
		//printf("%lld\n", msg1);
		if (-1 == buffer[tail])
			pthread_exit(NULL);
		tail++;
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
		if (head < tail) { // 아웃풋 버퍼가 비어있지 않을 때만 실행
			// 1.
			//msg2[0] = outputBuffer[t0][0];
			//msg2[1] = outputBuffer[t0][1];
			//msg2[2] = outputBuffer[t0][2];
			//printf("[current outputHead and outputTail : %d, %d], %lld, %lld, %lld\n", outputHead, outputTail, msg2[0], msg2[1], msg2[2]);

			// 2.
			//if (-1 == write(sock, msg2, sizeof(msg2)))
			//	error_handling("msg2 write() error");
			//write(sock, &msg2[0], sizeof(msg2[0]));
			//write(sock, &msg2[1], sizeof(msg2[1]));
			//write(sock, &msg2[2], sizeof(msg2[2]));
			cal();
			write(sock, msg, sizeof(msg));
			test_count2 = 0;
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
int led_breathing(){

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

int main(int argc, char **argv) {
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
	thr_id3 = pthread_create(&p_thread[2], NULL, cal, NULL);
	if (thr_id3 < 0) {
		perror("reveiving_thread created error : ");
		exit(0);
	}
	//cal();
	pthread_join(p_thread[0], (void **)&status);
	pthread_join(p_thread[1], (void **)&status);
	pthread_join(p_thread[2], (void **)&status);

	while(!fin);

	close(sock);


	return (0);
}
