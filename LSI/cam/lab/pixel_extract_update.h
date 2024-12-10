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

// the coordinate of the point in the image
typedef struct {
	int x;
	int y;
} Point;

// RGB Color space
typedef struct {
	int r;
	int g;
	int b;
} RGB;

// HSV Color space
typedef struct {
	double h;
	double s;
	double v;
} HSV;

// RGB Color table
RGB colors[6] = {
	{200, 50, 100}, // R
	{30, 170, 60}, // G
	{0, 160, 255}, // B
	{170, 200, 30}, // Y
	{200, 100, 50}, // O
	{180, 180, 180} // W
};

HSV color
int width, height, channels; // image info
unsigned char *image;

int plane[4]; // color info stream of each plane

Point center; // Center Position
Point p[4]; // p0, p1, p2, p3

int pl;


/* get pixel index of the point in the image */
int pixel_index(int x, int y) {
	int index = (y * width + x) * channels;
	return index;
}

/* get rgb value of the point */
RGB get_rgb(int x, int y) {
	RGB rgb;

	int idx = pixel_index(x, y);

	rgb.r = image[idx];
	rgb.g = image[idx + 1];
	rgb.b = image[idx + 2];

	printf("rgb: %d %d %d\n", rgb.r, rgb.g, rgb.b);

	return rgb;
}

/* get hsv value from rgb */
HSV get_hsv(RGB rgb) {
	HSV;
	double HSV;
	return HSV;

/* get abs diff between two */
int get_diff(int x, int y) {
	printf("diff: %d\n", abs(x-y));

	return abs(x-y);
}

/* get Euclidean RGB distance */
double get_dist(RGB c1, RGB c2) {
	double dist = sqrt( pow((c1.r - c2.r), 2) + pow((c1.g - c2.g), 2) + pow((c1.b - c2.b), 2) );
	return dist;
}

/* classify RGB color in 6 choices */
int judge_color(int red, int green, int blue) {
	int color_code;
	double dist;
	RGB c;
	c.r = red;
	c.g = green;
	c.b = blue;

	//double diff;
	//double wr=0.3, wg=0.59, wb=0.11;

	//int diff;
	double min = 9999;
	printf("===============================\n");

	for(int i=0; i<6; i++) {
		//diff = wr * get_diff(red, colors[i].r);
		//diff += wg * get_diff(green, colors[i].g);
		//diff += wb * get_diff(blue, colors[i].b);
		//printf("%d\n",diff);
		dist = get_dist(c, colors[i]);
		if(min > dist) {
			min = dist;
			color_code = i;
		}
	}

	return color_code;
}


/* init image */
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

/* pick a random point around the point */
Point pick_point(int x, int y, int dist) {
	Point rndp;
	rndp.x = (x-dist)+(rand()%(2*dist+1)); // x-dist ~ x+dist
	rndp.y = (y-dist)+(rand()%(2*dist+1)); // y-dist ~ y+dist
	return rndp;
}

/* process a plane of the cube */
void process_plane(void) {
	RGB c;
	int code;
	Point rndp;

	for(int i=0; i<4; i++) {
		int count[6] = {0}; // color count
		for(int j=0; j<1000; j++) {
			rndp = pick_point(p[i].x, p[i].y, RNDIST);
			c = get_rgb(rndp.x, rndp.y);
			code = judge_color(c.r, c.g, c.b);
			count[code]++;
		}

		int max = 0;
		int mode_idx = 0;
		for(int k=0; k<6; k++) {
			if(max < count[k]) {
				max = count[k];
				mode_idx = k;
			}
		}

		plane[i] = mode_idx;
	}	
}

/* flip the plane position, because the camera is flipped */
void flip(void) {
	int tmp;

	tmp = plane[1];
	plane[1] = plane[0];
	plane[0] = tmp;

	tmp = plane[3];
	plane[3] = plane[2];
	plane[2] = tmp;
}

/* load the captured image file, extract colors, and save to cube planar */
int process_image(void) {
	image = stbi_load("img.jpg", &width, &height, &channels, 0);
	if (!image) {
		printf("Failed to load image!\n");
		return 0;
	}
	init_image(CENTER_X, CENTER_Y, DIST);
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
