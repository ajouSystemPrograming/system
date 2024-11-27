#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define POUT 17 // GPIO 17 핀을 LED 출력에 사용
#define VALUE_MAX 256
#define DIRECTION_MAX 256
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define BUFFER_MAX 3

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

int main(int argc, char **argv) {
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
    	return 1;
	}

	if (GPIODirection(POUT, OUT) == -1) {
    	return 2;
	}

	int t0;
	while (1) {
		printf("value: %d\n", t0=readadc(fd, 0));
    if(t0<500) // t0은 0 부터 1023 사이 값이라는듯?
	    GPIOWrite(POUT, 0);
    else
	    GPIOWrite(POUT, 1);
	    usleep(10000);
	}
	
	if (GPIOUnexport(POUT) == -1) {
    	return 4;
	}


}
