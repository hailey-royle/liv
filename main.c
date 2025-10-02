#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define CURSOR_HOME "\x1b[H"
#define ERASE_SCREEN "\x1b[2J"
#define COLUMN_OFFSET 4
#define BOTTOM_OFFSET 2

struct termios NormalTermios;

struct piece {
    char* start;
    int length;
};

struct table {
    struct piece piece[256];
    char* original;
    char* added;
    int pieceCount;
    int linePiece;
    int lineStart;
    int lineLength;
    int lineNumber;
    int lineCursor;
};
struct table table;

struct liv {
    char* fileName;
    int columns;
    int rows;
};
struct liv liv;

void ValidateArgs(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Format: $ liv <fileName>");
        exit(1);
    }
    liv.fileName = argv[1];
}

void SetLineLength() {
    table.lineLength = 0;
    while (*(table.piece[table.linePiece].start + table.lineStart + table.lineLength) != '\n') {
        table.lineLength++;
    }
    if (table.lineLength > 0) {
        table.lineLength--;
    }
}

void LoadFile() {
    size_t size = 0;
    FILE* filePointer = fopen(liv.fileName, "r");
    if (filePointer == NULL) {
        printf("File: '%s' not found", liv.fileName);
        exit(1);
    }
    int fileLength = getdelim(&table.original, &size, '\0', filePointer);
    fclose(filePointer);
    table.piece[0].start = table.original;
    table.piece[0].length = fileLength;
    table.lineNumber++;
    SetLineLength();
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
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    liv.columns = ws.ws_col - COLUMN_OFFSET;
    liv.rows = ws.ws_row - BOTTOM_OFFSET;
}

void InitScreen() {
    EnableRawMode();
    GetScreenSize();
}

void GetLine(char* buffer, int remaining, int piece, int offset) {
    if (piece < 0) {
        *buffer = '\n';
        buffer++;
        *buffer = '\0';
        return;
    }
    while (*(table.piece[piece].start + offset) != '\n') {
        remaining--;
        if (remaining <= 1) {
            break;
        }
        if (offset >= table.piece[piece].length) {
            if (piece >= table.pieceCount) {
                break;
            }
            piece++;
            offset = 0;
        }
        *buffer = *(table.piece[piece].start + offset);
        buffer++;
        offset++;
    }
    *buffer = '\0';
}

void GetLineRelitive(char* buffer, int relitivity) {
    int piece = table.linePiece;
    int offset = table.lineStart;
    while (relitivity > 0) {
        int initPiece = piece;
        int initOffset = offset;
        while (*(table.piece[piece].start + offset) != '\n') {
            if (offset == table.piece[piece].length) {
                piece++;
                offset = 0;
            }
            offset++;
        }
        offset++;
        if (piece == table.pieceCount && table.piece[table.pieceCount].length <= offset) {
            piece = initPiece;
            offset = initOffset;
            return;
        }
        relitivity--;
    }
    while (relitivity < 0) {
        if (offset == 0 && piece == 0) {
            piece--;
            relitivity = 0;
            break;
        }
        if (offset != 0) {
            if (piece != 0) {
                piece--;
                offset = table.piece[piece].length;
            }
            offset--;
        }
        if (*(table.piece[piece].start + offset) == '\n') {
            if (offset != 0) {
                if (piece != 0) {
                    piece--;
                    offset = table.piece[piece].length;
                }
                offset--;
            }
        }
        while (*(table.piece[piece].start + offset) != '\n') {
            if (offset == 0) {
                if (piece == 0) {
                    break;
                }
                piece--;
                offset = table.piece[table.linePiece].length;
            }
            offset--;
        }
        if (!(offset == 0 && piece == 0)) {
            offset++;
        }
        relitivity++;
    }
    GetLine(&buffer[COLUMN_OFFSET], liv.columns, piece, offset);
}

void FormatScreenLine(char* buffer, int row) {
    if (row == liv.rows / 2) {
        sprintf(buffer, "%-3d ", table.lineNumber);
    } else {
        sprintf(buffer, "%3d ", abs(row - (liv.rows / 2)));
    }
    GetLineRelitive(buffer, row - (liv.rows / 2));
}

void RefreshScreen() {
    printf(CURSOR_HOME);
    printf(ERASE_SCREEN);
    for (int row = 1; row <= liv.rows; row++) {
        char buffer[liv.columns + COLUMN_OFFSET + 4];
        FormatScreenLine(buffer, row);
        printf("\x1b[%d;0H%s", row, buffer);
    }
    printf("\x1b[%d;0H-%s-", liv.rows + 1, liv.fileName);
    printf("\x1b[%d;0Hliv :)", liv.rows + 2);
    printf("\x1b[%d;%dH", liv.rows / 2, table.lineCursor + COLUMN_OFFSET + 1);
    fflush(stdout);
}

void LineNext() {
    int start = table.lineStart;
    int piece = table.linePiece;
    while (*(table.piece[table.linePiece].start + table.lineStart) != '\n') {
        if (table.lineStart == table.piece[table.linePiece].length) {
            table.linePiece++;
            table.lineStart = 0;
        }
        table.lineStart++;
    }
    table.lineStart++;
    if (table.linePiece == table.pieceCount && table.piece[table.pieceCount].length <= table.lineStart) {
        table.lineStart = start;
        table.linePiece = piece;
        return;
    }
    table.lineNumber++;
    table.lineCursor = 0;
    SetLineLength();
}

void LinePrevious() {
    if (table.lineStart != 0) {
        if (table.linePiece != 0) {
            table.linePiece--;
            table.lineStart = table.piece[table.linePiece].length;
        }
        table.lineStart--;
    }
    if (*(table.piece[table.linePiece].start + table.lineStart) == '\n') {
        if (table.lineStart != 0) {
            if (table.linePiece != 0) {
                table.linePiece--;
                table.lineStart = table.piece[table.linePiece].length;
            }
            table.lineStart--;
        }
    }
    while (*(table.piece[table.linePiece].start + table.lineStart) != '\n') {
        if (table.lineStart == 0) {
            if (table.linePiece == 0) {
                break;
            }
            table.linePiece--;
            table.lineStart = table.piece[table.linePiece].length;
        }
        table.lineStart--;
    }
    if (!(table.lineStart == 0 && table.linePiece == 0)) {
        table.lineStart++;
    }
    if (table.lineNumber > 1) {
        table.lineNumber--;
    }
    table.lineCursor = 0;
    SetLineLength();
}

void CursorLeft() {
    if (table.lineCursor > 0) { table.lineCursor--; }
}

void CursorRight() {
    if (table.lineCursor < table.lineLength) { table.lineCursor++; }
}

void ProssesKeyPress() {
    char key;
    read(STDIN_FILENO, &key, 1);
    if      (key == 'q') { exit(0); }
    else if (key == 'j') { LineNext(); }
    else if (key == 'k') { LinePrevious(); }
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
    InitScreen();
    RunLiv();
    return 0;
}
