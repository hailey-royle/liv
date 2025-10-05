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

struct termios NormalTermios;

struct piece {
    char* start;
    int length;
};

struct table {
    struct piece piece[256];
    char* original;
    char* added;
    int addedLength;
    int pieceCount;
    int linePiece;
    int lineStart;
    int lineNumber;
    int lineCursor;
};
struct table table;

struct liv {
    char* fileName;
    int columns;
    int rows;
    int centerRow;
};
struct liv liv;

void BufferAppend(char* buffer, int* bufferLength, char* append, int appendLength) {
    char* new = realloc(buffer, *bufferLength + appendLength);
    if (new == NULL) {
        return;
    }
    memcpy(&new[*bufferLength], append, appendLength);
    buffer = new;
    *bufferLength += appendLength;
}

void FindLineNext(int* originalPiece, int* originalOffset) {
    int piece = *originalPiece;
    int offset = *originalOffset;
    while (*(table.piece[piece].start + offset) != '\n') {
        if (table.piece[piece].length <= offset) {
            piece++;
            offset = 0;
        }
        offset++;
    }
    offset++;
    if (table.pieceCount <= piece && table.piece[table.pieceCount].length <= offset) {
        return;
    }
    *originalPiece = piece;
    *originalOffset = offset;
}

void FindLinePrevious(int* originalPiece, int* originalOffset) {
    int piece = *originalPiece;
    int offset = *originalOffset;
    for (int i = 0; i < 2; i++) {
        if (offset == 0) {
            if (piece == 0) {
                break;
            }
            piece--;
            offset = table.piece[piece].length;
        }
        offset--;
    }
    while (*(table.piece[piece].start + offset) != '\n') {
        if (offset == 0) {
            if (piece == 0) {
                break;
            }
            piece--;
            offset = table.piece[piece].length;
        }
        offset--;
    }
    if (!(offset == 0 && piece == 0)) {
        offset++;
    }
    *originalPiece = piece;
    *originalOffset = offset;
}

void MoveLineRelitive(int relitivity) {
    while (relitivity > 0) {
        int originalPiece = table.linePiece;
        int originalOffset = table.lineStart;
        FindLineNext(&table.linePiece, &table.lineStart);
        if (table.linePiece != originalPiece || table.lineStart != originalOffset) {
            table.lineNumber++;
        }
        relitivity--;
    }
    while (relitivity < 0) {
        FindLinePrevious(&table.linePiece, &table.lineStart);
        if (table.lineNumber > 1) {
            table.lineNumber--;
        }
        relitivity++;
    }
    table.lineCursor = 0;
}

void GetLine(char* buffer, int remaining, int piece, int offset) {
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
    int relitivePiece = table.linePiece;
    int relitiveOffset = table.lineStart;
    if (relitivity + table.lineNumber <= 0) {
        return;
    }
    while (relitivity > 0) {
        int originalPiece = relitivePiece;
        int originalOffset = relitiveOffset;
        FindLineNext(&relitivePiece, &relitiveOffset);
        relitivity--;
        if (relitivePiece == originalPiece && relitiveOffset <= originalOffset) {
            return;
        }
    }
    while (relitivity < 0) {
        FindLinePrevious(&relitivePiece, &relitiveOffset);
        relitivity++;
    }
    GetLine(buffer, liv.columns + 1, relitivePiece, relitiveOffset);
}

void CursorLeft() {
    if (table.lineCursor > 0) {
        table.lineCursor--;
    }
}

void CursorRight() {
    if (*(table.piece[table.linePiece].start + table.lineStart + table.lineCursor) != '\n' && table.lineCursor < liv.columns - 2) {
        table.lineCursor++;
    }
}

void ValidateArgs(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Format: $ liv <fileName>");
        exit(1);
    }
    liv.fileName = argv[1];
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
    liv.columns = ws.ws_col - COLUMN_OFFSET + 1;
    liv.rows = ws.ws_row;
    liv.centerRow = (liv.rows / 2) + 1;
}

void InitLiv(int argc, char* argv[]) {
    ValidateArgs(argc, argv);
    LoadFile();
    EnableRawMode();
    GetScreenSize();
}

void RefreshScreen() {
    printf(ERASE_SCREEN);
    for (int row = 1; row <= liv.rows; row++) {
        char buffer[liv.columns + 1];
        GetLineRelitive(buffer, row - liv.centerRow);
        if (row == liv.centerRow) {
            printf("\x1b[%d;0H%-3d %s", row, table.lineNumber, buffer);
        } else {
            printf("\x1b[%d;0H%3d %s", row, abs(row - liv.centerRow), buffer);
        }
    }
    printf("\x1b[%d;%dH", liv.centerRow, table.lineCursor + COLUMN_OFFSET + 1);
    fflush(stdout);
}

void ProssesKeyPress() {
    char key;
    read(STDIN_FILENO, &key, 1);
    if      (key == 'q') { exit(0); }
    else if (key == 'j') { MoveLineRelitive(1); }
    else if (key == 'k') { MoveLineRelitive(-1); }
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
    InitLiv(argc, argv);
    RunLiv();
    return 0;
}
