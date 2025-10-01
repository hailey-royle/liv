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

struct termios NormalTermios;

struct piece {
    char* buffer;
    int start;
    int length;
};

struct table {
    struct piece piece[256];
    char* original;
    char* added;
    char* currentLine;
    int currentLineNumber;
};

struct liv {
    struct table table;
    char* fileName;
    int columns;
    int rows;
    int cursorX;
};
struct liv liv;

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
    int fileLength = getdelim(&liv.table.original, &size, '\0', filePointer);
    fclose(filePointer);
    liv.table.piece[0].start = 0;
    liv.table.piece[0].length = fileLength;
    liv.table.piece[0].buffer = liv.table.original;
    liv.table.currentLine = liv.table.original;
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
    liv.columns = ws.ws_col - 4;
    liv.rows = ws.ws_row - 2;
}

void InitScreen() {
    EnableRawMode();
    GetScreenSize();
}

char* GetRelitiveLine(int offset) { // ignores pieces
    char* line = liv.table.currentLine;
    if (offset + liv.table.currentLineNumber < 0) {
        return NULL;
    }
    while (offset < 0) {
        line -= 2;
        while (line[0] != '\n') {
            line--;
        }
            line++;
        offset++;
    }
    while (offset > 0) {
        while (line[0] != '\n') {
            line++;
            if (line[0] == '\0') {
                return NULL;
            }
        }
        line++;
        offset--;
    }
    return line;
}

void RefreshScreen() {
    printf(CURSOR_HOME);
    printf(ERASE_SCREEN);
    for (int row = 0; row <= liv.rows; row++) {
        char* line = GetRelitiveLine(row - (liv.rows / 2));
        int lineLength = 0;
        if (line != NULL) {
            lineLength = strcspn(line, "\n");
        }
        if (row != liv.rows / 2) {
            printf("\x1b[%d;0H%3d %.*s", row, abs(row - (liv.rows / 2)), lineLength, line);
        } else {
            printf("\x1b[%d;0H%-3d %.*s", row, liv.table.currentLineNumber, strcspn(liv.table.currentLine, "\n"), liv.table.currentLine);
        }
    }
    printf("\x1b[%d;0H-%s-", liv.rows + 1, liv.fileName);
    printf("\x1b[%d;0Hliv :)", liv.rows + 2);
    printf("\x1b[%d;%dH", liv.rows / 2, liv.cursorX + 5);
    fflush(stdout);
}

void LineNext() {
    char* newLine = GetRelitiveLine(1);
    if (newLine) {
        liv.table.currentLine = newLine;
        liv.table.currentLineNumber++;
    }
}

void LinePrevious() {
    char* newLine = GetRelitiveLine(-1);
    if (newLine) {
        liv.table.currentLine = newLine;
        liv.table.currentLineNumber--;
    }
}

void CursorLeft() {
    if (liv.cursorX > 0) { liv.cursorX--; }
}

void CursorRight() {
    if (liv.cursorX < liv.columns - 1) { liv.cursorX++; }
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
