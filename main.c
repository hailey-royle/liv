#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "piecechain.h"

#define START_ALT_SCREEN "\x1b[?1049h"
#define END_ALT_SCREEN "\x1b[?1049l"
#define ERASE_SCREEN "\x1b[2J"

#define ESCAPE 27
#define BACKSPACE 127
#define NEWLINE 13

#define ORIGINAL 1
#define APPEND 0

struct insert {
    char* text;
    int mode;
    int removeCount;
};

struct liv {
    struct chain chain;
    struct insert insert;
    char* fileName;
    int rows;
    int columns;
    int columnOffset;
    int lineNumber;
    int cursor;
};

struct liv liv;
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
    write(STDOUT_FILENO, message, sizeof(message));
    exit(0);
}

void ValidateArgs(int argc, char* argv[]) {
    if (argc != 2) LivExit("wrong args, format: liv <filename>");
    liv.fileName = argv[1];
}

void LoadFile() {
    size_t size = 0;
    char* tempBuffer;
    FILE* fp = fopen(liv.fileName, "r");
    if (fp == NULL) LivExit("LoadFile fp is NULL");
    size = getdelim(&tempBuffer, &size, '\0', fp);
    if (size == -1) LivExit("LoadFile getdelim failed");
    liv.chain = InitChain(tempBuffer);
    free(tempBuffer);
    fclose(fp);
}

void LoadScreen() {
    struct winsize winsize;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
    liv.columns = winsize.ws_col;
    liv.rows = winsize.ws_row;
    liv.columnOffset = 4;
    liv.lineNumber = 1;
    liv.cursor = 1;
}

void WriteScreen() {
    printf(ERASE_SCREEN);
    for (int i = 1; i <= liv.rows; i++) {
        char buffer[liv.columns - liv.columnOffset + 1] = {};
        GetLine(&liv.chain, buffer, liv.columns - liv.columnOffset + 1, i - (liv.rows / 2) + liv.lineNumber);
        if (i - (liv.rows / 2) + liv.lineNumber < 1 || i - (liv.rows / 2) + liv.lineNumber > GetLineCount(&liv.chain)) {
            printf("\x1b[%d;0H~", i);
        } else if (i == liv.rows / 2) {
            printf("\x1b[%d;0H%-*d %s", i, liv.columnOffset - 1, liv.lineNumber, buffer);
        } else {
            printf("\x1b[%d;0H%*d %s", i, liv.columnOffset - 1, abs(i - (liv.rows / 2)), buffer);
        }
    }
    printf("\x1b[%d;%dH", (liv.rows / 2), liv.cursor + liv.columnOffset);
    fflush(stdout);
}


void LineNext() {
    if (liv.lineNumber < GetLineCount(&liv.chain)) {
        liv.lineNumber++;
        liv.cursor = 1;
    }
}

void LinePrev() {
    if (liv.lineNumber > 1) {
        liv.lineNumber--;
        liv.cursor = 1;
    }
}

void CursorPrev() {
    if (liv.cursor > 1) {
        liv.cursor--;
    }
}

void CursorNext() {
    if (liv.cursor < GetLineLength(&liv.chain, liv.lineNumber)) {
        liv.cursor++;
    }
}

void EnterInsert() {
    liv.insert.mode = 1;
    liv.insert.removeCount = 0;
}

void InsertChar(char key) {
    if (key == ESCAPE) {
        ModifyChain(&liv.chain, "", liv.lineNumber, liv.cursor, liv.insert.removeCount);
        liv.insert.mode = 0;
        liv.insert.removeCount = 0;
    } else if (key == BACKSPACE) {
        if (liv.cursor == 1) {
            if (liv.lineNumber > 1) {
                LinePrev();
                liv.insert.removeCount++;
                liv.cursor += GetLineLength(&liv.chain, liv.lineNumber);
                ModifyChain(&liv.chain, "", liv.lineNumber, liv.cursor, liv.insert.removeCount);
                liv.insert.removeCount = 0;
            }
        } else {
            liv.insert.removeCount++;
            liv.cursor--;
        }
    } else if (key == NEWLINE) {
    } else {
    }
}
void ProssesKeyPress() {
    char key;
    read(STDIN_FILENO, &key, 1);
    if (liv.insert.mode) {
        InsertChar(key);
        return;
    }
    if      (key == 'q') LivExit("Success!");
    else if (key == 'i') EnterInsert();
    else if (key == 'k') LinePrev();
    else if (key == 'j') LineNext();
    else if (key == 'h') CursorPrev();
    else if (key == 'l') CursorNext();
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
    LoadScreen();
    RunLiv();
}
