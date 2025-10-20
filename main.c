#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define ERASE_SCREEN "\x1b[2J"

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
    int columnOffset;
    int lineNumber;
    int lineLength;
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

void ColumnOffset(int lines) {
    while (lines > 0) {
        lines /= 10;
        liv.columnOffset++;
    }
    liv.columnOffset++;
    if (liv.columnOffset < 4) {
        liv.columnOffset = 4;
    }
}

void InitScreen() {
    EnableRawMode();
    GetScreenSize();
    ColumnOffset(table.lineCount);
    liv.lineNumber = 1;
    liv.cursor = 1;
}

void WriteLine(char* buffer, int count, int line) {
    if (line < 1 || line > table.lineCount) {
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
    for (int row = 1; row <= liv.rows; row++) {
        char buffer[liv.columns - liv.columnOffset + 1] = {};
        WriteLine(buffer, liv.columns - liv.columnOffset + 1, row - (liv.rows / 2) + liv.lineNumber);
        if (row < (liv.rows / 2)) {
            printf("\x1b[%d;0H%*d %s", row, liv.columnOffset - 1, abs(row - (liv.rows / 2)), buffer);
        } else if (row > (liv.rows / 2)) {
            printf("\x1b[%d;0H%*d %s", row, liv.columnOffset - 1, row - (liv.rows / 2), buffer);
        } else {
            printf("\x1b[%d;0H%-*d %s", row, liv.columnOffset - 1, liv.lineNumber, buffer);
            liv.lineLength = strlen(buffer);
        }
    }
    printf("\x1b[%d;%dH", (liv.rows / 2), liv.cursor + liv.columnOffset);
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
    if (liv.cursor < liv.columns - liv.columnOffset && liv.cursor < liv.lineLength) {
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
