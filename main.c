#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define ERASE_SCREEN "\x1b[2J"

#define BACKSPACE 127
#define NEWLINE 13

#define ORIGINAL 1
#define APPEND 0

struct piece {
    int next;
    int prev;
    int offset;
    int length;
};

struct chain {
    char* buffer;
    struct piece* piece;
    size_t bufferLength;
    int pieceCount;
};

struct screen {
    int rows;
    int columns;
    int columnOffset;
    int lineNumber;
    int lineLength;
    int cursor;
};

struct liv {
    struct screen s;
    struct chain c;
    char* fileName;
};

struct liv l;
struct termios NormalTermios;

void EnableRawMode() {
    write(STDOUT_FILENO, START_ALT_SCREEN, 8);
    tcgetattr(STDIN_FILENO, &NormalTermios);
    struct termios RawTermios = NormalTermios;
    RawTermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    RawTermios.c_oflag &= ~(OPOST);
    RawTermios.c_cflag |= (CS8);
    RawTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &RawTermios);
}

void DisableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &NormalTermios);
    write(STDOUT_FILENO, END_ALT_SCREEN, 8);
}

void LivExit(char* message) {
    DisableRawMode();
    printf("%s\n", message);
    exit(0);
}

void ValidateArgs(int argc, char* argv[]) {
    if (argc != 2) LivExit("wrong args, format: liv <filename>");
    l.fileName = argv[1];
}

void LoadFile() {
    FILE* fp = fopen(l.fileName, "r");
    if (fp == NULL) LivExit("LoadFile fp is NULL");
    l.c.bufferLength = getdelim(&l.c.buffer, &l.c.bufferLength, '\0', fp);
    if (l.c.bufferLength == -1) LivExit("LoadFile getdelim failed");
    fclose(fp);
}

void LoadChain() {
    l.c.piece = realloc(l.c.piece, sizeof(struct piece) * 3);
    if (l.c.piece == NULL) LivExit("LoadChain realloc failed");
    l.c.piece[0].next = 2;
    l.c.piece[0].prev = -1;
    l.c.piece[1].next = -1;
    l.c.piece[1].prev = 2;
    l.c.piece[2].next = 1;
    l.c.piece[2].prev = 0;
    l.c.piece[2].offset = 0;
    l.c.piece[2].length = l.c.bufferLength;
}

void LoadScreen() {
    struct winsize winsize;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
    l.s.columns = winsize.ws_col;
    l.s.rows = winsize.ws_row;
    l.s.columnOffset = 4;
    l.s.lineNumber = 1;
    l.s.cursor = 1;
}

void FindLine(int* piece, int* offset, int line) {
    while (line > 1) {
        if (l.c.piece[*piece].next == -1) {
            *piece = -1;
            *offset = 0;
            break;
        }
        if (*offset > l.c.piece[*piece].length) {
            *offset = 0;
            *piece = l.c.piece[*piece].next;
            continue;
        }
        if (l.c.buffer[l.c.piece[*piece].offset + *offset] == '\n') {
            line--;
        }
        *offset += 1;
    }
}

void WriteLine(char* buffer, int count, int line) {
    int piece = 0;
    int offset = 0;
    if (line < 1) return;
    FindLine(&piece, &offset, line);
    if (piece == -1) return;
    while (count > 1) {
        if (l.c.piece[piece].next == -1) break;
        if (offset > l.c.piece[piece].length) {
            offset = 0;
            piece = l.c.piece[piece].next;
            continue;
        }
        if (l.c.buffer[l.c.piece[piece].offset + offset] == '\n') break;
        *buffer = l.c.buffer[l.c.piece[piece].offset + offset];
        buffer++;
        offset++;
        count--;
    }
    *buffer = '\0';
}

void WriteScreen() {
    printf(ERASE_SCREEN);
    for (int i = 1; i <= l.s.rows; i++) {
        char buffer[l.s.columns - l.s.columnOffset + 1] = {};
        WriteLine(buffer, l.s.columns - l.s.columnOffset + 1, i - (l.s.rows / 2) + l.s.lineNumber);
        if (i == l.s.rows / 2) {
            printf("\x1b[%d;0H%-*d %s", i, l.s.columnOffset - 1, l.s.lineNumber, buffer);
            l.s.lineLength = strlen(buffer);
        } else {
            printf("\x1b[%d;0H%*d %s", i, l.s.columnOffset - 1, abs(i - (l.s.rows / 2)), buffer);
        }
    }
    printf("\x1b[%d;%dH", (l.s.rows / 2), l.s.cursor + l.s.columnOffset);
    fflush(stdout);
}

void LineDown() {
    int piece = 0;
    int offset = 0;
    FindLine(&piece, &offset, l.s.lineNumber);
    if (piece != -1) {
        l.s.lineNumber++;
        l.s.cursor = 1;
    }
}

void LineUp() {
    if (l.s.lineNumber > 1) {
        l.s.lineNumber--;
        l.s.cursor = 1;
    }
}

void CursorLeft() {
    if (l.s.cursor > 1) {
        l.s.cursor--;
    }
}

void CursorRight() {
    if (l.s.cursor < l.s.columns - l.s.columnOffset && l.s.cursor < l.s.lineLength) {
        l.s.cursor++;
    }
}

void ProssesKeyPress() {
    char key;
    read(STDIN_FILENO, &key, 1);
    if      (key == 'q') LivExit("Success!");
    else if (key == 'j') LineDown();
    else if (key == 'k') LineUp();
    else if (key == 'h') CursorLeft();
    else if (key == 'l') CursorRight();
}

void RunLiv() {
    while (true) {
        WriteScreen();
        ProssesKeyPress();
    }
}

int main(int argc, char* argv[]) {
    EnableRawMode();
    ValidateArgs(argc, argv);
    LoadFile();
    LoadChain();
    LoadScreen();
    RunLiv();
}
