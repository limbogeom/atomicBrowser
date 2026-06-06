#pragma once
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

class Terminal {
private:
    inline static termios orig_termios;

public:
    static void enableRawMode() {
        tcgetattr(STDIN_FILENO, &orig_termios);
        termios raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    static void disableRawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }

    static int getWidth() {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        return w.ws_col;
    }

    enum Keys { UP = 1000, DOWN, RIGHT, LEFT, ENTER = 10, TAB = 9, ESC = 27, BACKSPACE = 127 };

    static int readKey() {
        char c = '\0';
        if (read(STDIN_FILENO, &c, 1) == 0) return 0;

        if (c == '\x1b') {
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) return ESC;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return ESC;
            if (seq[0] == '[') {
                if (seq[1] == 'A') return UP;
                if (seq[1] == 'B') return DOWN;
                if (seq[1] == 'C') return RIGHT;
                if (seq[1] == 'D') return LEFT;
            }
            return ESC;
        }
        return c;
    }
};
