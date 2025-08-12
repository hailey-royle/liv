#include <stdio.h>
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

void InitLv() {
    printf("\x1B[H\x1B[22J");
    printf("\x1B[38;2;214;192;201m");
    printf("\x1B[48;2;26;21;24m");
    EnableRawMode();
}

void DisableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void ExitLv() {
    DisableRawMode();
    printf("\x1B[0m");
    printf("\x1B[H\x1B[22J");
}

int main() {

    InitLv();

    char c;
    while (1) {
        char c = '\0';
        read(STDIN_FILENO, &c, 1);

        if (c > 32 && c < 127) {
            printf("%d ('%c')\r\n\x1B[1;1H\x1B[2K", c, c);
        } else {
            printf("%d\r\n\x1B[1;1H\x1B[2K", c);
        }
        if (c == 'q') break;
    }

    ExitLv();

    return 0;
}
