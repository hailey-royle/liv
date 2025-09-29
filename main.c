#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define CURSOR_HOME "\x1b[H"
#define ERASE_SCREEN "\x1b[2J"

struct termios NormalTermios;

struct screen {
    int columns;
    int rows;
    int cursorX;
    int cursorY;
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
    printf(START_ALT_SCREEN);
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
    printf(CURSOR_HOME);
    printf(ERASE_SCREEN);
    for (int i = 0; i < screen.rows; i++) {
        printf("%2d", i);
        if (i < screen.rows - 1) {
            printf("\r\n");
        }
        fflush(stdout);
    }
    printf("\x1b[%d;%dH", screen.cursorY + 1, screen.cursorX + 1);
    fflush(stdout);
}

void CursorLeft() {
    if (screen.cursorX > 0) {
        screen.cursorX--;
    }
}

void CursorDown() {
    if (screen.cursorY < screen.rows) {
        screen.cursorY++;
    }
}

void CursorUp() {
    if (screen.cursorY > 0) {
        screen.cursorY--;
    }
}

void CursorRight() {
    if (screen.cursorX < screen.columns) {
        screen.cursorX++;
    }
}

void ProssesKeyPress() {
    char key;
    read(STDIN_FILENO, &key, 1);
    if      (key == 'q') { ExitLiv(); }
    else if (key == 'h') { CursorLeft(); }
    else if (key == 'j') { CursorDown(); }
    else if (key == 'k') { CursorUp(); }
    else if (key == 'l') { CursorRight(); }
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
