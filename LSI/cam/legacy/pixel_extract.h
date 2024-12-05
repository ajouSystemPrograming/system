#ifndef __PIXEL_EXTRACT_H__
#define __PIXEL_EXTRACT_H__

#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CENTER_X 320
#define CENTER_Y 240
#define DIST 100
#define RNDIST 20

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

Color colors[6] = {
	{200, 50, 100},
	{30, 170, 60},
	{0, 160, 255},
	{170, 200, 30},
	{200, 100, 50},
	{180, 180, 180}
};

int width, height, channels; // image info
unsigned char *image;

int plane[4]; // color info stream of each plane

Point center; // Center Position
Point p[4]; // p0, p1, p2, p3

int pl;

Point pick_point(int x, int y, int dist);

void process_plane(void);

void flip(void);

int process_image(void);

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

	printf("rgb: %d %d %d\n", rgb.r, rgb.g, rgb.b);

	return rgb;
}

int get_diff(int x, int y) {
	printf("diff: %d\n", abs(x-y));

	return abs(x-y);
}

int judge_color(int red, int green, int blue) {
	int color_code;
	int diff;
	int min = 999;
	printf("===============================\n");

	for(int i=0; i<6; i++) {
		diff = get_diff(red, colors[i].r);
		diff += get_diff(green, colors[i].g);
		diff += get_diff(blue, colors[i].b);
		printf("%d\n",diff);
		if(min > diff) {
			min = diff;
			color_code = i;
		}
	}

	return color_code;
}



int init_image(int center_x, int center_y, int dist) {
	srand(time(NULL));

	center.x = center_x;
	center.y = center_y;

	for(int i=0; i<4; i++) {
		p[i].x = (i%2==0) ? center_x - dist : center_x + dist;
		p[i].y = (i<2) ? center_y + dist : center_y - dist;
	}

	int i0;
	mask[0] = 1;
	for(i0 = 1; i0 < 24; i0++)
		mask[i0] = mask[i0-1]*6;
}

Point pick_point(int x, int y, int dist) {
	Point rndp;
	rndp.x = (x-dist)+(rand()%(2*dist+1)); // x-dist ~ x+dist
	rndp.y = (y-dist)+(rand()%(2*dist+1)); // y-dist ~ y+dist
	return rndp;
}

void process_plane(void) {
	Color c, fin;
	int code;
	Point rndp;

	for(int i=0; i<4; i++) {
		int tr=0, tg=0, tb=0;
		for(int j=0; j<10; j++) {
			rndp = pick_point(p[i].x, p[i].y, RNDIST);
			c = get_rgb(rndp.x, rndp.y);
			tr += c.r; tg += c.g; tb += c.b;
			printf("total: %d %d %d\n", tr, tg, tb);
		}
		fin.r = tr/10; fin.g = tg/10; fin.b = tb/10;
		printf("color: %d %d %d\n", fin.r, fin.g, fin.b);
		code = judge_color(fin.r, fin.g, fin.b);
		plane[i] = code;
	}	
}

void flip(void) {
	int tmp;

	tmp = plane[1];
	plane[1] = plane[0];
	plane[0] = tmp;

	tmp = plane[3];
	plane[3] = plane[2];
	plane[2] = tmp;
}


int process_image(void) {
	image = stbi_load("img.jpg", &width, &height, &channels, 0);
	if (!image) {
		printf("Failed to load image!\n");
		return 0;
	}
	process_plane();
	flip();
	for(int k=0; k<4; k++) {
		printf("%d", plane[k]);
		cube[order[pl*4+k][0]][order[pl*4+k][1]] = plane[k];
	}
	printf("\n");

	stbi_image_free(image); // Free the image data
	return 0;
}

#endif
