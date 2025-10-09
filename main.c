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
    int insertMode;
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

void ShiftPieceTable(int shift) {
    for (int piece = table.pieceCount; piece > table.linePiece; piece--) {
        table.piece[piece + shift] = table.piece[piece];
    }
    table.pieceCount += shift;
}

void SplitPiece(int shift) {
    ShiftPieceTable(shift);
    table.piece[table.linePiece + shift].start = table.piece[table.linePiece].start + table.lineCursor;
    table.piece[table.linePiece + shift].length = table.piece[table.linePiece].length - table.lineCursor;
    table.piece[table.linePiece].length = table.lineCursor;
}

void InsertChar(char key) { // remove then add case
    if (key == '\b') {
        if (table.piece[table.linePiece].length == 0) {
        } else {
            table.piece[table.linePiece].length--;
        }
    } else {
        BufferAppend(table.added, &table.addedLength, &key, 1);
    }
}

void EnterInsertMode() {
    liv.insertMode = 1;
    SplitPiece(2);
    table.linePiece++;
    table.piece[table.linePiece].length = 0;
    table.piece[table.linePiece].start = table.added + table.addedLength;
}

void FindLineNext(int* originalPiece, int* originalOffset) {
    int piece = *originalPiece;
    int offset = *originalOffset;
    while (*(table.piece[piece].start + offset) != '\n') {
        if (offset >= table.piece[piece].length) {
            piece++;
            offset = 0;
        }
        offset++;
    }
    offset++;
    if (offset < table.piece[piece].length) {
        *originalPiece = piece;
        *originalOffset = offset;
    }
}

void FindLinePrevious(int* originalPiece, int* originalOffset) {
    int piece = *originalPiece;
    int offset = *originalOffset;
    for (int i = 0; i < 2; i++) {
        if (offset == 0) {
            if (piece == 1) {
                break;
            }
            piece--;
            offset = table.piece[piece].length;
        }
        offset--;
    }
    while (*(table.piece[piece].start + offset) != '\n') {
        if (offset == 0) {
            if (piece == 1) {
                break;
            }
            piece--;
            offset = table.piece[piece].length;
        }
        offset--;
    }
    if (!(offset == 0 && piece == 1)) {
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
            if (piece >= table.pieceCount - 1) {
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
        printf("Format: $ liv <fileName>\n");
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
    table.piece[0].start = table.added;
    table.piece[0].length = 0;
    table.piece[1].start = table.original;
    table.piece[1].length = fileLength;
    table.piece[2].start = table.added;
    table.piece[2].length = 0;
    table.pieceCount = 2;
    table.linePiece = 1;
    table.lineNumber = 1;
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

void RefreshLineCenter() {
    char buffer[liv.columns + 1];
    GetLine(buffer, liv.columns + 1, table.linePiece, table.lineStart);
    printf("\x1b[%d;0H%-3d %s", liv.centerRow, table.lineNumber, buffer);
}

void RefreshLinesPrevious() {
    char buffer[liv.columns + 1];
    int piece = table.linePiece;
    int offset = table.lineStart;
    for (int row = liv.centerRow - 1; row > 0; row--) {
        int startPiece = piece;
        int startOffset = offset;
        FindLinePrevious(&piece, &offset);
        if (startPiece != piece || startOffset != offset) {
            GetLine(buffer, liv.columns + 1, piece, offset);
        } else {
            buffer[0] = '\0';
        }
        printf("\x1b[%d;0H%3d %s", row, abs(row - liv.centerRow), buffer);
    }
}

void RefreshLinesNext() {
    char buffer[liv.columns + 1];
    int piece = table.linePiece;
    int offset = table.lineStart;
    for (int row = liv.centerRow + 1; row <= liv.rows; row++) {
        int startPiece = piece;
        int startOffset = offset;
        FindLineNext(&piece, &offset);
        if (startPiece != piece || startOffset != offset) {
            GetLine(buffer, liv.columns + 1, piece, offset);
        } else {
            buffer[0] = '\0';
        }
        printf("\x1b[%d;0H%3d %s", row, abs(row - liv.centerRow), buffer);
    }
}

void RefreshScreen() {
    printf(ERASE_SCREEN);
    RefreshLineCenter();
    RefreshLinesNext();
    RefreshLinesPrevious();
    printf("\x1b[%d;%dH", liv.centerRow, table.lineCursor + COLUMN_OFFSET + 1);
    fflush(stdout);
}

void ProssesKeyPress() {
    char key;
    read(STDIN_FILENO, &key, 1);
    if (liv.insertMode) {
        if      (key == '\e'){ liv.insertMode = 0; }
        else                 { InsertChar(key); }
    } else {
        if      (key == 'q') { exit(0); }
        else if (key == 'i') { EnterInsertMode(); }
        else if (key == 'j') { MoveLineRelitive(1); }
        else if (key == 'k') { MoveLineRelitive(-1); }
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
    InitLiv(argc, argv);
    RunLiv();
    return 0;
}
