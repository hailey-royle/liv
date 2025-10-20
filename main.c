#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define ERASE_SCREEN "\x1b[2J"
#define COLUMN_OFFSET 4
#define CENTER_ROW (liv.rows / 2) + 1

struct termios NormalTermios;

struct piece {
    char* buffer;
    int offset;
    int length;
};

struct buffer {
    char* data;
    int length;
};

struct table {
    struct piece* piece;
    struct buffer original;
    struct buffer added;
    int pieceCount;
    int lineCount;
};
struct table table;

struct liv {
    char* fileName;
    int columns;
    int rows;
    int lineNumber;
    int cursor;
};
struct liv liv;

void ValidateArgs(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Format: $ liv <fileName>\n");
        exit(1);
    }
    liv.fileName = argv[1];
}

void LoadFile() {
    size_t size = 0;
    FILE* fp = fopen(liv.fileName, "r");
    if (fp == NULL) {
        printf("File: '%s' not found", liv.fileName);
        exit(1);
    }
    table.original.length = getdelim(&table.original.data, &size, '\1', fp);
    fclose(fp);
    for (int i = 0; i < table.original.length; i++) {
        if (table.original.data[i] == '\n') {
            table.lineCount++;
        }
    }
}

void InitPieceTable() {
    struct piece* new = realloc(table.piece, sizeof(struct piece));
    if (new == NULL) { return; }
    table.piece = new;
    table.piece[0].buffer = table.original.data;
    table.piece[0].offset = 0;
    table.piece[0].length = table.original.length;
    table.pieceCount = 1;
}

void DisableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &NormalTermios);
    printf(END_ALT_SCREEN);
}

void EnableRawMode() {
    printf(START_ALT_SCREEN);
    tcgetattr(STDIN_FILENO, &NormalTermios);
    struct termios RawTermios = NormalTermios;
    RawTermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    RawTermios.c_oflag &= ~(OPOST);
    RawTermios.c_cflag |= (CS8);
    RawTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &RawTermios);
    atexit(DisableRawMode);
}

void GetScreenSize() {
    struct winsize winsize;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
    liv.columns = winsize.ws_col;
    liv.rows = winsize.ws_row;
}

void InitScreen() {
    EnableRawMode();
    GetScreenSize();
    liv.lineNumber = 1;
    liv.cursor = 1;
}

void WriteLine(char* buffer, int count, int line) {
    if (line < 1) {
        return;
    }
    if (line > table.lineCount) {
        return;
    }
    int piece = 0;
    int offset = 0;
    while (line > 1) {
        if (table.piece[piece].buffer[table.piece[piece].offset + offset] != '\n') {
            if (offset >= table.piece[piece].length) {
                piece++;
                offset = 0;
            } else {
                offset++;
            }
        } else {
            offset++;
            line--;
        }
    }
    while (table.piece[piece].buffer[table.piece[piece].offset + offset] != '\n' && count >= 1) {
        if (table.piece[piece].length < offset) {
            piece++;
            offset = 0;
        }
        *buffer = table.piece[piece].buffer[table.piece[piece].offset + offset];
        buffer++;
        offset++;
        count--;
    }
    *buffer = '\0';
}

void RefreshScreen() {
    printf(ERASE_SCREEN);
    for (int row = 1; row < liv.rows; row++) {
        char buffer[liv.columns - COLUMN_OFFSET + 1] = {};
        WriteLine(buffer, liv.columns - COLUMN_OFFSET + 1, row - CENTER_ROW + liv.lineNumber - 2);
        if (row < CENTER_ROW) {
            printf("\x1b[%d;0H%3d %s", row, abs(row - CENTER_ROW), buffer);
        } else if (row > CENTER_ROW) {
            printf("\x1b[%d;0H%3d %s", row, row - CENTER_ROW, buffer);
        } else {
            printf("\x1b[%d;0H%-3d %s", CENTER_ROW, liv.lineNumber, buffer);
        }
    }
    printf("\x1b[%d;%dH", CENTER_ROW, liv.cursor + COLUMN_OFFSET);
    fflush(stdout);
}

void LineUp() {
    if (liv.lineNumber > 1) {
        liv.lineNumber--;
        liv.cursor = 1;
    }
}

void LineDown() {
    if (liv.lineNumber < table.lineCount) {
        liv.lineNumber++;
        liv.cursor = 1;
    }
}

void CursorLeft() {
    if (liv.cursor > 1) {
        liv.cursor--;
    }
}

void CursorRight() {
    if (liv.cursor < liv.columns - COLUMN_OFFSET) {
        liv.cursor++;
    }
}

void ProssesKeyPress() {
    char key;
    read(STDIN_FILENO, &key, 1);
    if      (key == 'q') { exit(0); }
    else if (key == 'j') { LineDown(); }
    else if (key == 'k') { LineUp(); }
    else if (key == 'h') { CursorLeft(); }
    else if (key == 'l') { CursorRight(); }
}

void RunLiv() {
    while (1) {
        RefreshScreen();
        ProssesKeyPress();
    }
}

int main(int argc, char* argv[]) {
    ValidateArgs(argc, argv);
    LoadFile();
    InitPieceTable();
    InitScreen();
    RunLiv();
    return 0;
}
