#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define CURSOR_HOME "\x1b[H"
#define ERASE_SCREEN "\x1b[2J"
#define SAVE_CURSOR "\x1b[s"
#define RESTORE_CURSOR "\x1b[u"

struct termios NormalTermios;

struct screen {
    int columns;
    int rows;
};
struct screen screen;

void EnableRawMode() {
    tcgetattr(STDIN_FILENO, &NormalTermios);
    struct termios RawTermios = NormalTermios;
    RawTermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    RawTermios.c_oflag &= ~(OPOST);
    RawTermios.c_cflag |= (CS8);
    RawTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &RawTermios);
}

void GetWindowSize() {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    screen.columns = ws.ws_col;
    screen.rows = ws.ws_row;
}

void InitLiv() {
    write(STDOUT_FILENO, START_ALT_SCREEN, sizeof(START_ALT_SCREEN));
    EnableRawMode();
    GetWindowSize();
}

void DisableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &NormalTermios);
}

void ExitLiv() {
    DisableRawMode();
    printf(END_ALT_SCREEN);
    exit(0);
}

void RefreshScreen() {
    write(STDOUT_FILENO, SAVE_CURSOR, sizeof(SAVE_CURSOR));
    write(STDOUT_FILENO, CURSOR_HOME, sizeof(CURSOR_HOME));
    write(STDOUT_FILENO, ERASE_SCREEN, sizeof(ERASE_SCREEN));
    for (int i = 0; i < screen.rows; i++) {
        printf("%2d", i);
        if (i < screen.rows - 1) {
            printf("\r\n");
        }
        fflush(stdout);
    }
    write(STDOUT_FILENO, RESTORE_CURSOR, sizeof(RESTORE_CURSOR));
}

void ProssesKeyPress() {
    char key;
    read(STDIN_FILENO, &key, 1);
    if (key == 0) {
    } else if (key == 'q') {
        ExitLiv();
    } else if (key >= 32 && key < 127) {
    } else {
    }
}

void RunLiv() {
    while (1) {
        RefreshScreen();
        ProssesKeyPress();
    }
}

int main() {
    InitLiv();
    RunLiv();
    return 0;
}
