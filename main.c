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

#define NORMAL 0
#define INSERT 1

struct liv {
    struct chain chain;
    char* fileName;
    char* insert;
    int mode;
    int commandCount;
    int removeCount;
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
    write(STDOUT_FILENO, message, strlen(message));
    exit(0);
}

void ValidateArgs(int argc, char* argv[]) {
    if (argc != 2) LivExit("wrong args, format: liv <filename>\r\n");
    liv.fileName = argv[1];
}

void LoadFile() {
    size_t size = 0;
    char* tempBuffer;
    FILE* fp = fopen(liv.fileName, "r");
    if (fp == NULL) LivExit("LoadFile fp is NULL\r\n");
    size = getdelim(&tempBuffer, &size, '\0', fp);
    if (size == -1) LivExit("LoadFile getdelim failed\r\n");
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
    liv.insert = calloc(liv.columns - liv.columnOffset + 1, 1);
    if (liv.insert == NULL) LivExit("LoadScreen malloc failed\r\n");
}

void GetInsertTextLine(char* buffer, int bufferLength) {
    if (strlen(liv.insert) > liv.removeCount) {
        for (int i = strlen(buffer) + 2; i > liv.cursor - strlen(liv.insert); i--) {
            buffer[i + strlen(liv.insert) - liv.removeCount - 2] = buffer[i - 2];
        }
    } else if (strlen(liv.insert) < liv.removeCount) {
        for (int i = liv.cursor - 1; i < strlen(buffer); i++) {
            buffer[i] = buffer[i + liv.removeCount - strlen(liv.insert)];
        }
    }
    for (int i = 0; i < strlen(liv.insert); i++) {
        buffer[i + liv.cursor - strlen(liv.insert) - 1] = liv.insert[i];
    }
}

void WriteScreen() {
    printf(ERASE_SCREEN);
    for (int i = 1; i <= liv.rows; i++) {
        char buffer[liv.columns - liv.columnOffset + 1] = {};
        GetLine(&liv.chain, buffer, liv.columns - liv.columnOffset + 1, i - (liv.rows / 2) + liv.lineNumber);
        if (strlen(buffer) > 0) {
            buffer[strlen(buffer) - 1] = '\0';
        }
        if (i - (liv.rows / 2) + liv.lineNumber < 1 || i - (liv.rows / 2) + liv.lineNumber > GetLineCount(&liv.chain)) {
            printf("\x1b[%d;0H~", i);
        } else if (i == liv.rows / 2) {
            GetInsertTextLine(buffer, liv.columns - liv.columnOffset + 1);
            printf("\x1b[%d;0H%-*d %s", i, liv.columnOffset - 1, liv.lineNumber, buffer);
        } else {
            printf("\x1b[%d;0H%*d %s", i, liv.columnOffset - 1, abs(i - (liv.rows / 2)), buffer);
        }
    }
    printf("\x1b[%d;%dH", (liv.rows / 2), liv.cursor + liv.columnOffset);
    fflush(stdout);
}

void InsertChar(char key) {
    if (key == ESCAPE) {
        ModifyChain(&liv.chain, liv.insert, liv.lineNumber, liv.cursor - strlen(liv.insert), liv.removeCount);
        liv.mode = NORMAL;
        liv.removeCount = 0;
        liv.insert[0] = '\0';
    } else if (key == BACKSPACE) {
        if (strlen(liv.insert) > 0) {
            liv.insert[strlen(liv.insert) - 1] = '\0';
            liv.cursor--;
        } else if (liv.cursor == 1) {
            if (liv.lineNumber > 1) {
                liv.lineNumber--;
                liv.cursor = GetLineLength(&liv.chain, liv.lineNumber) + 1;
                liv.removeCount++;
                ModifyChain(&liv.chain, liv.insert, liv.lineNumber, liv.cursor, liv.removeCount);
                liv.removeCount = 0;
            }
        } else {
            liv.removeCount++;
            liv.cursor--;
        }
    } else if (key == NEWLINE) {
        liv.insert[strlen(liv.insert) + 1] = '\0';
        liv.insert[strlen(liv.insert)] = '\n';
        ModifyChain(&liv.chain, liv.insert, liv.lineNumber, liv.cursor - strlen(liv.insert) + 1, liv.removeCount);
        liv.removeCount = 0;
        liv.insert[0] = '\0';
        liv.lineNumber++;
        liv.cursor = 1;
    } else {
        liv.insert[strlen(liv.insert) + 1] = '\0';
        liv.insert[strlen(liv.insert)] = key;
        liv.cursor++;
    }
}

void WriteFile() {
    FILE* fp = fopen(liv.fileName, "w");
    if (fp == NULL) LivExit("WriteFile fp is NULL\r\n");
    for (int i = 1; i <= GetLineCount(&liv.chain); i++) {
        char buffer[liv.columns - liv.columnOffset + 1] = {};
        GetLine(&liv.chain, buffer, liv.columns - liv.columnOffset + 1, i);
        fprintf(fp, "%s", buffer);
    }
    fclose(fp);
}

void EnterInsert() {
    liv.mode = INSERT;
    liv.removeCount = 0;
    liv.insert[0] = '\0';
}

void EnterInsertAppend() {
    liv.cursor++;
    liv.mode = INSERT;
    liv.removeCount = 0;
    liv.insert[0] = '\0';
}

void CursorPrev() {
    while (liv.commandCount > 0) {
        if (liv.cursor > 1) {
            liv.cursor--;
        }
        liv.commandCount--;
    }
}

void CursorNext() {
    while (liv.commandCount > 0) {
        if (liv.cursor < GetLineLength(&liv.chain, liv.lineNumber)) {
            liv.cursor++;
        }
        liv.commandCount--;
    }
}

void WordPrev() {
    char buffer[liv.columns - liv.columnOffset + 1] = {};
    GetLine(&liv.chain, buffer, liv.columns - liv.columnOffset + 1, liv.lineNumber);
    while (liv.commandCount > 0) {
        while (buffer[liv.cursor - 1] == ' ' && liv.cursor > 1) {
            liv.cursor--;
        }
        while (buffer[liv.cursor - 1] != ' ' && liv.cursor > 1) {
            liv.cursor--;
        }
        liv.commandCount--;
    }
}

void WordNext() {
    char buffer[liv.columns - liv.columnOffset + 1] = {};
    GetLine(&liv.chain, buffer, liv.columns - liv.columnOffset + 1, liv.lineNumber);
    while (liv.commandCount > 0) {
        while (buffer[liv.cursor - 1] == ' ') {
            liv.cursor++;
        }
        while (buffer[liv.cursor - 1] != ' ' && buffer[liv.cursor] != '\n' && buffer[liv.cursor] != '\0') {
            liv.cursor++;
        }
        liv.commandCount--;
    }
}

void LineNext() {
    while (liv.commandCount > 0) {
        if (liv.lineNumber < GetLineCount(&liv.chain)) {
            liv.lineNumber++;
            liv.cursor = 1;
        }
        liv.commandCount--;
    }
}

void LinePrev() {
    while (liv.commandCount > 0) {
        if (liv.lineNumber > 1) {
            liv.lineNumber--;
            liv.cursor = 1;
        }
        liv.commandCount--;
    }
}

void FileStart() {
    liv.lineNumber = 1;
    liv.cursor = 1;
}

void FileEnd() {
    liv.lineNumber = GetLineCount(&liv.chain);
    liv.cursor = 1;
}

void ProssesKeyPress() {
    char key;
    read(STDIN_FILENO, &key, 1);
    if (liv.mode) {
        InsertChar(key);
        return;
    }
    if (key >= '0' && key <= '9') {
        liv.commandCount *= 10;
        liv.commandCount += (key & 0xf);
    } else {
        if (liv.commandCount == 0) {
            liv.commandCount = 1;
        }
        if      (key == 'q') LivExit("Success!\r\n");
        else if (key == 'w') WriteFile();
        else if (key == 'i') EnterInsert();
        else if (key == 'a') EnterInsertAppend();
        else if (key == 'h') CursorPrev();
        else if (key == 'l') CursorNext();
        else if (key == 'b') WordPrev();
        else if (key == 'e') WordNext();
        else if (key == 'k') LinePrev();
        else if (key == 'j') LineNext();
        else if (key == 't') FileStart();
        else if (key == 'z') FileEnd();
        liv.commandCount = 0;
    }
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
