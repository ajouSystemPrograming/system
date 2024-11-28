#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h> // v4l2 library header

// camera device file path
#define CAMERA "/dev/video0"

// the name of captured image
#define IMAGE "capture.jpg"

// image size (2592x1944 is maximum)
#define WIDTH 640
#define HEIGHT 480

uint8_t *buffer;

static int xioctl(int fd, int request, void *arg)
{
	int r;
	do r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);
	return r;
}

int main(void) {
	int fd;
	fd = open(CAMERA, O_RDWR);
	if (fd == -1)
	{
		// couldn't find capture device
		fprintf(stderr, "Failed to open camera device!");
		return 1;
	}

	struct v4l2_capability caps = {0};
	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &caps))
	{
		perror("Querying Capabilites");
		return 2;
	}

	struct v4l2_format fmt = {0};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = WIDTH;
	fmt.fmt.pix.height = HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
	{
		perror("Setting Pixel Format");
		return 1;
	}

	struct v4l2_requestbuffers req = {0};
	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
	{
		perror("Requesting Buffer");
		return 1;
	}

	struct v4l2_buffer buf = {0};
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0; // bufferindex;
	if(-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
	{
		perror("Querying Buffer");
		return 1;
	}

	buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
	if(-1 == xioctl(fd, VIDIOC_STREAMON, &buf.type))
	{
		perror("Start Capture");
		return 1;
	}

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	struct timeval tv = {0};
	tv.tv_sec = 2;
	int r = select(fd+1, &fds, NULL, NULL, &tv);
	if(-1 == r)
	{
		perror("Waiting for Frame");
		return 1;
	}

	if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf))
	{
		perror("Retrieving Frame");
		return 1;
	}

	FILE *image = fopen(IMAGE, "wb");
	if(!image) {
		fprintf(stderr, "Failed to write the image file!\n");
		exit(-1);
	}
	fwrite(buffer, buf.length, 1, image);
	fclose(image);

	if(-1 == xioctl(fd, VIDIOC_STREAMOFF, &buf.type)) {
		fprintf(stderr, "Failed to stop streaming!\n");
		exit(-1);
	}

	return 0;
}
