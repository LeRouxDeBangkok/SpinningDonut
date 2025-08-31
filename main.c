#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

void display(const char *output, int buffer_size, int width, float *a, float *b) {
    printf("\x1b[H");
    for  (int i = 0; i < buffer_size; i++) {
        putchar(i % width ? output[i] : '\n');
    }
    *a += 0.00004;
    *b += 0.00002;
}

void render (float a, float b, int buffer_size, float *zBuffer, char *output, int height, int width, int size) {
    int outer_size = size;
    char *lum_chars = ".,-~:;=!*#$@";
    memset(output, ' ', buffer_size);
    memset(zBuffer, 0, buffer_size * sizeof(float));

    for (float theta = 0; theta < 6.28; theta += 0.07) {
        for (float phi = 0; phi < 6.28; phi += 0.02) {
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);
            float cosTheta = cosf(theta);
            float sinTheta = sinf(theta);
            float sinA = sinf(a), cosA = cosf(a);
            float sinB = sinf(b), cosB = cosf(b);
            float circleX = cosTheta + outer_size;
            float circleY = sinPhi * circleX * cosA - sinTheta * sinA;
            float norm_z = 1 / (sinPhi * circleX * sinA + sinTheta * cosA + 5);

            int x = (width / 2) + (int)(30 * norm_z * (cosPhi * circleX * cosB + circleY * sinB));
            int y = (height / 2) + (int)(15 * norm_z * (cosPhi * circleX * sinB - circleY * cosB));
            int o = x + width * y;
            int lum = (int)(8 * ((sinTheta * sinA - sinPhi * cosTheta * cosA) * cosB - sinPhi * cosTheta * sinA - sinTheta * cosA - cosPhi * cosTheta * sinB));

            if (height > y && y >= 0 && x >= 0 && width > x && norm_z > zBuffer[o]) {
                zBuffer[o] = norm_z;
                output[o] = lum_chars[lum > 0 ? lum : 0];
            }
        }
    }
}

int main () {
    float a = 0, b = 0;
    int height = 22;
    int width = 80;
    int buffer_size = height * width;
    float *zBuffer = malloc(buffer_size * sizeof(float));
    char *output = malloc(buffer_size);

    int size;
    printf("Enter the size of the donut (recommended 3-6): ");
    scanf("%d", &size);

    printf("\x1b[2J");

    while (1) {
        render(a, b, buffer_size, zBuffer, output, height, width, size);
        display(output, buffer_size, width, &a, &b);
        usleep(30000);
    }

    free(zBuffer);
    free(output);
    return 0;
}