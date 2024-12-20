#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 256
#define VALUE_MAX 256

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define POUT 23
#define PIN 24

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

int main(int argc, char *argv[]) {
  int repeat = 9;
  clock_t start_t, end_t;
  double time;

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

  // init ultrawave trigger
  GPIOWrite(POUT, 0);
  usleep(10000);
  // start
  do {
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
    printf("distance : %.2lfcm\n", time / 2 * 34000);

    usleep(2000000);
  } while (repeat--);

  // Disable GPIO pins
  if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN)) return (4);

  printf("complete\n");

  return (0);
}

