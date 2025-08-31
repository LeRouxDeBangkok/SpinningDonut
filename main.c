#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif
static void pause_usec(unsigned int usec) {
#ifdef _WIN32
    Sleep((usec + 999) / 1000);
#else
    usleep(usec);
#endif
}
#ifdef _WIN32
static void enable_ansi_windows(void) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode;
    if (!GetConsoleMode(hOut, &mode)) return;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
}
#endif
static void get_terminal_size(int *rows, int *cols) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE ||
        !GetConsoleScreenBufferInfo(hOut, &csbi)) {
        *cols = 80; *rows = 24; // fallback
        return;
    }
    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        *cols = 80; *rows = 24; // fallback
    } else {
        *cols = w.ws_col;
        *rows = w.ws_row;
    }
#endif
}

void display(const char *output, int buffer_size, int width) {
    printf("\x1b[H"); // move cursor to top-left
    for (int i = 0; i < buffer_size; i++) {
        putchar(i % width ? output[i] : '\n');
    }
    fflush(stdout);
}

void render(float a, float b, int buffer_size, float *zBuffer, char *output,
            int height, int width, int size) {
    int outer_size = size;
    const char *lum_chars = ".,-~:;=!*#$@";
    const int LUM_LEN = (int)strlen(lum_chars);

    memset(output, ' ', buffer_size);
    memset(zBuffer, 0, buffer_size * sizeof(float));

    float scale_x = width / 2.2f;
    float scale_y = height / 1.1f;

    for (float theta = 0; theta < 6.28f; theta += 0.07f) {
        for (float phi = 0; phi < 6.28f; phi += 0.02f) {
            float sinPhi = sinf(phi), cosPhi = cosf(phi);
            float cosTheta = cosf(theta), sinTheta = sinf(theta);
            float sinA = sinf(a), cosA = cosf(a);
            float sinB = sinf(b), cosB = cosf(b);

            float circleX = cosTheta + outer_size;
            float circleY = sinPhi * circleX * cosA - sinTheta * sinA;
            float norm_z  = 1.0f / (sinPhi * circleX * sinA + sinTheta * cosA + 5.0f);

            int x = (width / 2) + (int)(scale_x * norm_z * (cosPhi * circleX * cosB + circleY * sinB));
            int y = (height / 2) + (int)(scale_y * norm_z * (cosPhi * circleX * sinB - circleY * cosB));

            if (y >= 0 && y < height && x >= 0 && x < width) {
                int o = x + width * y;

                int lum = (int)(8.0f * ((sinTheta * sinA - sinPhi * cosTheta * cosA) * cosB
                                       - sinPhi * cosTheta * sinA - sinTheta * cosA
                                       - cosPhi * cosTheta * sinB));
                int lumIdx = lum > 0 ? lum : 0;
                if (lumIdx >= LUM_LEN) lumIdx = LUM_LEN - 1;

                if (norm_z > zBuffer[o]) {
                    zBuffer[o] = norm_z;
                    output[o] = lum_chars[lumIdx];
                }
            }
        }
    }
}

int main(void) {
#ifdef _WIN32
    enable_ansi_windows();
#endif

    int height, width;
    get_terminal_size(&height, &width);
    if (height < 8) height = 8;
    if (width < 20) width = 20;

    int buffer_size = height * width;
    float *zBuffer = (float *)malloc(buffer_size * sizeof(float));
    char *output = (char *)malloc(buffer_size);
    if (!zBuffer || !output) {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    printf("\x1b[2J");

    float a = 0.0f, b = 0.0f;
    int size;
    printf("Enter the size of the donut (recommended 3 and fullscreen): ");
    if (scanf("%d", &size) != 1) size = 4;

    while (1) {
        render(a, b, buffer_size, zBuffer, output, height, width, size);
        display(output, buffer_size, width);
        a += 0.04f;
        b += 0.02f;

        pause_usec(30000);
    }

    free(zBuffer);
    free(output);
    return 0;
}