#include <stdio.h>
#include <stdlib.h>
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

int whole[BUF_MAX]; // whole planes color info stream
int plane[4]; // color info stream of each plane

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

char* tochar = "RGBYOW* EXYZ";
int frame[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};

static long long encode() {
	int i0;
	long long l0 = 0;
	for(i0 = 0; i0 < 24; i0++)
		l0 += cube[order[i0][0]][order[i0][1]]*mask[i0];
	return l0;
}

static int getColor(long long l0, int index) {
    return (l0/mask[index])%6;
}

static void decode(long long l0) {
    int i0;
    for(i0 = 0; i0 < 24; i0++)
        cube[order[i0][0]][order[i0][1]] = getColor(l0, i0);
}

void printCube(int r, int c) {
    int i0,i1;
    for(i0 = 0; i0 < 4; i0++)
        cube[r+frame[i0][0]][c+frame[i0][1]]=6;

    for(i0 = 0; i0 < 13; i0++)
    {
        for(i1 = 0; i1 < 17; i1++)
            printf("%c",tochar[cube[i0][i1]]);
        printf("\n");
    }
    for(i0 = 0; i0 < 4; i0++)
        cube[r+frame[i0][0]][c+frame[i0][1]]=7;
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



int init(int center_x, int center_y, int dist) {
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



int main() {
	long long planar;
	int tmp;


	init(CENTER_X, CENTER_Y, DIST);

	for(int i=0; i<6; i++) {
		printf("capture next and press enter!\n");
		tmp = getchar();
		image = stbi_load("img.jpg", &width, &height, &channels, 0);
		if (!image) {
			printf("Failed to load image!\n");
			return 0;
		}
		process_plane();
		for(int j=0; j<4; j++) {
			printf("%d", plane[j]);
		}
		printf("\n");

		flip();
		for(int k=0; k<4; k++) {
			printf("%d", plane[k]);
			cube[order[i*4+k][0]][order[i*4+k][1]] = plane[k];
		}
		printf("\n");
	}

	planar = encode();

	printf("%lld\n", planar);

	decode(planar);
	printCube(5, 1);

	stbi_image_free(image); // Free the image data
	return 0;
}
