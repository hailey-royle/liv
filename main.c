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

struct termios NormalTermios;

struct piece {
    int buffer;
    int offset;
    int length;
};

struct buffer {
    char* data;
    int length;
};

struct table {
    struct piece* piece;
    struct buffer buffer[2];
    int pieceCurrent;
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
    table.buffer[ORIGINAL].length = getdelim(&table.buffer[ORIGINAL].data, &size, '\1', fp);
    fclose(fp);
    for (int i = 0; i < table.buffer[ORIGINAL].length; i++) {
        if (table.buffer[ORIGINAL].data[i] == '\n') {
            table.lineCount++;
        }
    }
}

void InitPieceTable() {
    struct piece* new = realloc(table.piece, sizeof(struct piece));
    if (new == NULL) {
        printf("realloc error");
        exit(1);
    }
    table.piece = new;
    table.piece[0].buffer = ORIGINAL;
    table.piece[0].offset = 0;
    table.piece[0].length = table.buffer[ORIGINAL].length;
    table.pieceCount = 1;
    table.pieceCurrent = -1;
    table.buffer[APPEND].data = malloc(1);
    if (table.buffer[APPEND].data == NULL) {
        printf("malloc error");
        exit(1);
    }
    table.buffer[APPEND].data[0] = '\0';
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

void FindLine(int* piece, int* offset, int line) {
    while (line > 1) {
        if (*offset >= table.piece[*piece].length) {
            *piece += 1;
            *offset = 0;
            continue;
        }
        if (table.buffer[table.piece[*piece].buffer].data[table.piece[*piece].offset + *offset] == '\n') {
            line--;
        }
        *offset += 1;
    }
}

void WriteLine(char* buffer, int count, int line) {
    if (line < 1 || line > table.lineCount) {
        return;
    }
    int piece = 0;
    int offset = 0;
    FindLine(&piece, &offset, line);
    while (count >= 1) {
        if (table.piece[piece].length <= offset) {
            piece++;
            offset = 0;
            continue;
        }
        if (table.buffer[table.piece[piece].buffer].data[table.piece[piece].offset + offset] == '\n') {
            break;
        }
        *buffer = table.buffer[table.piece[piece].buffer].data[table.piece[piece].offset + offset];
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
        fflush(stdout);
    }
    printf("\x1b[%d;%dH", (liv.rows / 2), liv.cursor + liv.columnOffset);
    fflush(stdout);
}

void InsertExit() {
    table.pieceCurrent = -1;
}

void InsertChar(char key) {
    if (key == BACKSPACE || key == NEWLINE) {
        exit(1);
    }
    table.buffer[APPEND].data = realloc(table.buffer[APPEND].data, table.buffer[APPEND].length + 1);
    if (table.buffer[APPEND].data == NULL) {
        printf("realloc error");
        exit(1);
    }
    table.buffer[APPEND].data[table.buffer[APPEND].length] = key;
    table.buffer[APPEND].length += 1;
    table.piece[table.pieceCurrent].length++;
    liv.cursor++;
    liv.lineLength++;
}

void FindLineCursor(int* piece, int* offset, int line, int cursor) {
    FindLine(piece, offset, line);
    while (cursor > 1) {
        if (table.piece[*piece].length < *offset) {
            *piece += 1;
            *offset = 0;
        }
        *offset += 1;
        cursor--;
    }
}

void SplitPiece(int splitPiece, int splitOffset) {
    table.pieceCount += 2;
    struct piece* new = realloc(table.piece, table.pieceCount * sizeof(struct piece));
    if (new == NULL) {
        printf("realloc error");
        exit(1);
    }
    table.piece = new;
    for (int i = table.pieceCount - 3; i > splitPiece; i--) {
        table.piece[i + 2] = table.piece[i];
    }
    table.piece[splitPiece + 2].buffer = table.piece[splitPiece].buffer;
    table.piece[splitPiece + 2].offset = table.piece[splitPiece].offset + splitOffset;
    table.piece[splitPiece + 2].length = table.piece[splitPiece].length - splitOffset;
    table.piece[splitPiece + 1].buffer = APPEND;
    table.piece[splitPiece + 1].offset = table.buffer[APPEND].length;
    table.piece[splitPiece + 1].length = 0;
    table.piece[splitPiece].length = splitOffset;
}

void InsertInit() {
    int piece = 0;
    int offset = 0;
    FindLineCursor(&piece, &offset, liv.lineNumber, liv.cursor);
    SplitPiece(piece, offset);
    table.pieceCurrent = piece + 1;
}

void LineDown() {
    if (liv.lineNumber < table.lineCount) {
        liv.lineNumber++;
        liv.cursor = 1;
    }
}

void LineUp() {
    if (liv.lineNumber > 1) {
        liv.lineNumber--;
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
    if (table.pieceCurrent != -1) {
        if (key == 0x1b) { InsertExit(); }
        else { InsertChar(key); }
    } else {
        if      (key == 'q') { exit(0); }
        else if (key == 'i') { InsertInit(); }
        else if (key == 'j') { LineDown(); }
        else if (key == 'k') { LineUp(); }
        else if (key == 'h') { CursorLeft(); }
        else if (key == 'l') { CursorRight(); }
    }
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
