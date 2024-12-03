#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CENTER_X 320
#define CENTER_Y 240
#define DIST 10

#define TOLERANCE 30

#define BUF_MAX 25

typedef struct {
	int x;
	int y;
} Point;

typedef struct {
	int r;
	int g;
	int b;
} Color;

typedef enum {
	RED, // 230, 90, 90
	GREEN, // 80, 250, 80
	BLUE, // 100, 200, 255
	YELLOW, // 220, 255, 30
	ORANGE, // 255, 130, 35
	WHITE, // 255, 255, 255
	UNKNOWN = -1
} ColorCode;

Color colors[6] = {
	{230, 90, 90},
	{80, 250, 80},
	{100, 200, 255},
	{220, 255, 30},
	{255, 130, 35},
	{255, 255, 255}
};

int width, height, channels; // image info
unsigned char *image;

int tolerance = TOLERANCE; // error allowance range

int whole[BUF_MAX]; // whole planes color info stream
int plane[5]; // color info stream of each plane

Point center; // Center Position
Point p[4]; // p0, p1, p2, p3


static char cube[13][17] = {
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,8,7,7,7,5,7,1,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,4,7,3,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,2,7,5,7,2,7,0,7,1,7,5,7,4,7,0,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,4,7,4,7,1,7,0,7,2,7,1,7,5,7,2,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,3,7,3,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,3,7,0,7,7,7,7,7,7,7,7,7},
	{7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7}
};

int order[24][2] = {{5,1},{5,3},{7,1},{7,3},{5,5},{5,7},{7,5},{7,7},{5,9},{5,11},{7,9},{7,11},{5,13},{5,15},{7,13},{7,15},{1,5},{1,7},{3,5},{3,7},{9,5},{9,7},{11,5},{11,7}};

static long long mask[24];

static long long encode() {
	int i0;
	long long l0 = 0;
	for(i0 = 0; i0 < 24; i0++)
		l0 += cube[order[i0][0]][order[i0][1]]*mask[i0];
	return l0;
}



int pixel_index(int x, int y) {
	int index = (y * width + x) * channels; // Calculate the pixel's index
	return index;
}

Color get_rgb(int x, int y) {
	Color rgb;

	int idx = pixel_index(x, y);

	rgb.r = image[idx];
	rgb.g = image[idx + 1];
	rgb.b = image[idx + 2];

	return rgb;
}

int get_diff(int x, int y) {
	return abs(x-y);
}

ColorCode judge_color(int red, int green, int blue) {
	ColorCode color_code;
	Color diff;

	for(int i=0; i<=6; i++) {
		diff.r = get_diff(red, colors[i].r);
		diff.g = get_diff(green, colors[i].g);
		diff.b = get_diff(blue, colors[i].b);

		if(diff.r <= tolerance && diff.g <= tolerance && diff.b <= tolerance) {
			color_code = i;
			return color_code;
		}
	}

	color_code = RED; // if unknown, set to RED
	return color_code;
}


int init(int center_x, int center_y, int dist) {
	center.x = center_x;
	center.y = center_y;

	for(int i=0; i<4; i++) {
		p[i].x = (i%2==0) ? center_x - dist : center_x + dist;
		p[i].y = (i<2) ? center_y + dist : center_y - dist;
	}
}

void process_plane(void) {
	Color c;
	ColorCode code;

	for(int i=0; i<4; i++) {
		c = get_rgb(p[i].x, p[i].y);
		code = judge_color(c.r, c.g, c.b);
		plane[i] = code;
	}	
}


int main() {
	long long planar;
	int tmp;
	image = stbi_load("capture.jpg", &width, &height, &channels, 0);

	if (!image) {
		printf("Failed to load image!\n");
		return 1;
	}

	init(CENTER_X, CENTER_Y, DIST);

	for(int i=0; i<6; i++) {
		printf("capture next and press enter!\n");
		tmp = getchar();
		process_plane();
		for(int j=0; j<4; j++) {
			printf("%d", plane[j]);
			cube[order[i*4+j][0]][order[i*4+j][1]] = plane[j];
		}
		printf("\n");
	}

	planar = encode();

	printf("%lld\n", planar);

	stbi_image_free(image); // Free the image data
	return 0;
}
