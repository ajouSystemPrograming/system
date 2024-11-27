#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main() {
    int width, height, channels;
    unsigned char *image_data = stbi_load("example_red.png", &width, &height, &channels, 0);
    if (!image_data) {
        printf("Failed to load image\n");
        return 1;
    }

    int x = 50, y = 100; // Pixel position
    int pixel_index = (y * width + x) * channels; // Calculate the pixel's index
    unsigned char r = image_data[pixel_index];
    unsigned char g = image_data[pixel_index + 1];
    unsigned char b = image_data[pixel_index + 2];

    printf("The color at (%d, %d) is RGB(%d, %d, %d)\n", x, y, r, g, b);

    stbi_image_free(image_data); // Free the image data
    return 0;
}
