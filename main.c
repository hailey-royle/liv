#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

struct termios orig_termios;

void EnableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void InitLiv() {
    printf("\x1B[H\x1B[22J");
    fflush(stdout);
    EnableRawMode();
}

void DisableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void ExitLiv() {
    DisableRawMode();
    printf("\x1B[H\x1B[22J");
    exit(0);
}

void RefreshScreen() {
    printf("\x1B[H\x1B[22J");
}

void ProsessKey() {
    char key = '\0';
    read(STDIN_FILENO, &key, 1);

    if (key == 'q') ExitLiv();
    if (key == 'l') {
        printf("\rliv");
        fflush(stdout);
    }
}

int main() {

    InitLiv();

    while (1) {
        ProsessKey();
    }

    return 0;
}
