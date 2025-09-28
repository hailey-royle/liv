#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"

struct termios NormalTermios;

void EnableRawMode() {
    tcgetattr(STDIN_FILENO, &NormalTermios);
    struct termios RawTermios = NormalTermios;
    RawTermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    RawTermios.c_oflag &= ~(OPOST);
    RawTermios.c_cflag |= (CS8);
    RawTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &RawTermios);
}

void InitLiv() {
    write(STDOUT_FILENO, START_ALT_SCREEN, sizeof(START_ALT_SCREEN));
    EnableRawMode();
}

void DisableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &NormalTermios);
}

void ExitLiv() {
    DisableRawMode();
    printf(END_ALT_SCREEN);
    exit(0);
}

void ProssesKeyPress() {
    char key;
    read(STDIN_FILENO, &key, 1);
    if (key == 0) {
    } else if (key == 'q') {
        ExitLiv();
    } else if (key >= 32 && key < 127) {
        printf("(%d):%c\r\n", key, key);
    } else {
        printf("(%d):\r\n", key);
    }
}

void RunLiv() {
    while (1) {
        ProssesKeyPress();
    }
}

int main() {
    InitLiv();
    RunLiv();
    return 0;
}
