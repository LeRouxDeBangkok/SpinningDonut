#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

void display(const char *output, int buffer_size, int width) {
    printf("\x1b[H"); // move cursor to top-left
    for (int i = 0; i < buffer_size; ++i) {
        putchar(i % width ? output[i] : '\n');
    }
}

void render(float a, float b, int buffer_size, float *zBuffer, char *output, int height, int width, int size) {
    int outer_size = size;
    const char *lum_chars = ".,-~:;=!*#$@";
    const int lum_chars_len = 12;
    memset(output, ' ', buffer_size);
    memset(zBuffer, 0, buffer_size * sizeof(float));
    float scale_x = width * 0.35f;
    float scale_y = height * 0.55f;

    for (float theta = 0.0f; theta < 6.28f; theta += 0.07f) {
        for (float phi = 0.0f; phi < 6.28f; phi += 0.02f) {
            float sinPhi = sinf(phi), cosPhi = cosf(phi);
            float cosTheta = cosf(theta), sinTheta = sinf(theta);
            float sinA = sinf(a), cosA = cosf(a);
            float sinB = sinf(b), cosB = cosf(b);
            float circleX = cosTheta + outer_size;
            float circleY = sinPhi * circleX * cosA - sinTheta * sinA;
            float norm_z = 1.0f / (sinPhi * circleX * sinA + sinTheta * cosA + 5.0f);

            int x = (int)((width  / 2.0f) + (scale_x * norm_z * (cosPhi * circleX * cosB + circleY * sinB)));
            int y = (int)((height / 2.0f) + (scale_y * norm_z * (cosPhi * circleX * sinB - circleY * cosB)));
            if (x < 0 || x >= width || y < 0 || y >= height) continue;
            int o = x + width * y;
            int lum = (int)(8.0f * ((sinTheta * sinA - sinPhi * cosTheta * cosA) * cosB
                        - sinPhi * cosTheta * sinA - sinTheta * cosA
                        - cosPhi * cosTheta * sinB));
            int li = lum;
            if (li < 0) li = 0;
            if (li >= lum_chars_len) li = lum_chars_len - 1;

            if (norm_z > zBuffer[o]) {
                zBuffer[o] = norm_z;
                output[o] = lum_chars[li];
            }
        }
    }
}

int main(void) {
    float a = 0.0f, b = 0.0f;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    int width = 80, height = 25;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hOut, &csbi)) {
        width  = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
        height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (width <= 0) width = 80;
        if (height <= 0) height = 25;
    }

    int buffer_size = width * height;
    float *zBuffer = (float*)malloc(buffer_size * sizeof(float));
    char  *output  = (char*) malloc(buffer_size);
    if (!zBuffer || !output) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    DWORD mode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    int size;
    printf("Enter the size of the donut (recommended 3-8): ");
    if (scanf("%d", &size) != 1) size = 5;
    printf("\x1b[2J");

    for (;;) {
        render(a, b, buffer_size, zBuffer, output, height, width, size);
        display(output, buffer_size, width);
        a += 0.04f;
        b += 0.02f;

        Sleep(30);
    }

    free(zBuffer);
    free(output);
    return 0;
}
