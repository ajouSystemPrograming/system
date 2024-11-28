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

int main(void) {
	// step 1: open camera device
	int fd;
	fd = open(CAMERA, O_RDWR);

	if (fd == -1) {
		// couldn't find capture device
		fprintf(stderr, "Failed to open camera device!\n");
		return 1;
	}


	// step 2: capability check
	struct v4l2_capability caps = {0};

	if (ioctl(fd, VIDIOC_QUERYCAP, &caps) == -1) {
		fprintf(stderr, "Failed to query capabilities!\n");
		return 1;
	}

	// step 3: set formats
	struct v4l2_format fmt = {0};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = WIDTH;
	fmt.fmt.pix.height = HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
		fprintf(stderr, "Failed to set media formats!\n");
		return 1;
	}


	// step 4: request buffers
	struct v4l2_requestbuffers req = {0};
	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
		fprintf(stderr, "Failed to request buffers!\n");
		return 1;
	}

	// step 5: query & queue buffers
	struct v4l2_buffer buf = {0};
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0; // bufferindex;

	if(ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
		fprintf(stderr, "Failed to query buffers!\n");
		return 1;
	}

	void *buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

	if(ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
		fprintf(stderr, "Failed to queue buffers!\n");
		return 1;
	}


	// step 6: get ready for capture
	if(ioctl(fd, VIDIOC_STREAMON, &buf.type) == -1)	{
		fprintf(stderr, "Failed to start capture!\n");
		return 1;
	}

	// step 7: capture the frame
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	struct timeval tv = {0};
	tv.tv_sec = 2;

	int r = select(fd+1, &fds, NULL, NULL, &tv);

	if(r == -1) {
		fprintf(stderr, "Failed to wait for frame!\n");
		return 1;
	}


	// step 8: deque the buffer
	if(ioctl(fd, VIDIOC_DQBUF, &buf) == -1)	{
		fprintf(stderr, "Failed to retrieve captured frame!\n");
		return 1;
	}


	// step 9: save the image file
	int image = open(IMAGE, O_RDWR | O_CREAT, 0666);
	write(image, buffer, buf.length);


	// step 10: 
	if(ioctl(fd, VIDIOC_STREAMOFF, &buf.type) == -1) {
		fprintf(stderr, "Failed to stop streaming!\n");
		exit(-1);
	}

	return 0;
}
