#ifndef __SERVO_H__
#define __SERVO_H__

#define MOTOR_0 1450000
#define MOTOR_90 400000

#define IN 0
#define OUT 1
#define PWM 0

#define LOW 0
#define HIGH 1
#define VALUE_MAX 256
#define DIRECTION_MAX 256

#define EXPORT_PATH "/sys/class/pwm/pwmchip0/export"
#define UNEXPORT_PATH "/sys/class/pwm/pwmchip0/unexport"
#define ENABLE_PATH "/sys/class/pwm/pwmchip0/pwm0/enable"
#define PERIOD_PATH "/sys/class/pwm/pwmchip0/pwm0/period"
#define DUTY_PATH "/sys/class/pwm/pwmchip0/pwm0/duty_cycle"

static int PWMExport(int pwmnum) {
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	int fd, byte;

	fd = open(EXPORT_PATH, O_WRONLY);
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

static int PWMUnexport(int pwmnum) {
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open(UNEXPORT_PATH, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}


static int PWMEnable(int pwmnum) {
	static const char s_enable_str[] = "1";

	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, ENABLE_PATH, pwmnum);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in enable!\n");
		return -1;
	}

	write(fd, s_enable_str, strlen(s_enable_str));
	close(fd);

	return (0);
}

static int PWMDisable(int pwmnum) {
	static const char s_enable_str[] = "0";

	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, ENABLE_PATH, pwmnum);
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

	snprintf(path, VALUE_MAX, PERIOD_PATH, pwmnum);
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

	snprintf(path, VALUE_MAX, DUTY_PATH, pwmnum);
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

int spin(int servo, int val) {
	// PWMEnable(servo);
	PWMWriteDutyCycle(servo, val);
	// PWMDisable(servo);
	usleep(1000000);
	PWMWriteDutyCycle(servo, MOTOR_0);
	usleep(1000000);
	return 0;
}

#endif

