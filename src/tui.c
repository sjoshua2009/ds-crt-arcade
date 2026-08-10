#include "tui.h"

#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static struct termios g_orig;
static bool g_raw = false;

void tui_init(void) {
    /* 非 TTY 环境（如管道输入）下 tcgetattr 会失败，此时不做 raw 设置 */
    if (tcgetattr(STDIN_FILENO, &g_orig) == 0) {
        struct termios raw = g_orig;
        raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
        raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        raw.c_cflag |= CS8;
        raw.c_oflag &= ~OPOST;
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0)
            g_raw = true;
    }
    printf("\x1b[?25l");
    fflush(stdout);
}

void tui_restore(void) {
    if (g_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig);
        g_raw = false;
    }
    printf("\x1b[?25h\x1b[0m");
    fflush(stdout);
}

void tui_clear(void) {
    printf("\x1b[2J\x1b[H");
    fflush(stdout);
}

void tui_move(int row, int col) {
    printf("\x1b[%d;%dH", row, col);
}

void tui_set(int style) {
    switch (style) {
        case TUI_GREEN_DIM: printf("\x1b[2;32m"); break;
        case TUI_GREEN:     printf("\x1b[32m");    break;
        case TUI_GREEN_BRIGHT: printf("\x1b[1;92m"); break;
        case TUI_YELLOW:    printf("\x1b[1;93m");  break;
        case TUI_RED:       printf("\x1b[1;91m");  break;
        case TUI_INVERSE:   printf("\x1b[30;102m"); break;
        default:            printf("\x1b[0m");
    }
    fflush(stdout);
}

static void put_utf8(const char *s) {
    fputs(s, stdout);
}

void tui_box(int r0, int c0, int r1, int c1) {
    tui_set(TUI_GREEN);
    tui_move(r0, c0);     put_utf8("╔");
    for (int c = c0 + 1; c < c1; c++) put_utf8("═");
    put_utf8("╗");
    for (int r = r0 + 1; r < r1; r++) {
        tui_move(r, c0); put_utf8("║");
        tui_move(r, c1); put_utf8("║");
    }
    tui_move(r1, c0);     put_utf8("╚");
    for (int c = c0 + 1; c < c1; c++) put_utf8("═");
    put_utf8("╝");
    fflush(stdout);
}

void tui_hline(int r, int c0, int c1, int style) {
    tui_set(style);
    tui_move(r, c0);
    for (int c = c0; c <= c1; c++) put_utf8("─");
    fflush(stdout);
}

void tui_text(int r, int c, const char *s) {
    tui_move(r, c);
    put_utf8(s);
    fflush(stdout);
}

/* 定位输出，但截断到不超过终端右边界；不切断多字节 UTF-8 字符 */
void tui_put_safe(int r, int c, const char *s) {
    int max = tui_term_cols() - c;
    if (max <= 0) return;
    int n = 0;
    while (s[n] && n < max) n++;
    while (n > 0 && (s[n] & 0xC0) == 0x80) n--;
    if (n == 0) return;
    char buf[256];
    memcpy(buf, s, (size_t)n);
    buf[n] = '\0';
    tui_text(r, c, buf);
}

/* 支持环境变量 TUI_COLS/TUI_ROWS 覆盖（小终端测试或手动指定尺寸用） */
static int term_override(const char *env) {
    const char *e = getenv(env);
    if (e && *e) {
        int v = atoi(e);
        if (v > 0) return v;
    }
    return 0;
}

int tui_term_cols(void) {
    int v = term_override("TUI_COLS");
    if (v) return v;
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        return w.ws_col;
    return 80;
}

int tui_term_rows(void) {
    int v = term_override("TUI_ROWS");
    if (v) return v;
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0)
        return w.ws_row;
    return 24;
}

int tui_center_x(int width) {
    int c = (tui_term_cols() - width) / 2;
    return c < 1 ? 1 : c;
}

int tui_center_row(int height) {
    int r = (tui_term_rows() - height) / 2;
    return r < 1 ? 1 : r;
}

static int read_byte(void) {
    unsigned char b;
    ssize_t n = read(STDIN_FILENO, &b, 1);
    return n == 1 ? (int)b : -1;
}

/* 尝试读取跟在 ESC 后面的一个字节，最多等待 ms 毫秒 */
static int read_byte_after(int ms) {
    struct pollfd p = {STDIN_FILENO, POLLIN, 0};
    if (poll(&p, 1, ms) <= 0) return -1;
    return read_byte();
}

Key tui_getch_timeout(int ms) {
    struct pollfd p = {STDIN_FILENO, POLLIN, 0};
    int pr = poll(&p, 1, ms);
    if (pr <= 0) return KEY_NONE;
    int b = read_byte();
    if (b < 0) return KEY_NONE;
    switch (b) {
        case '\n': case '\r': return KEY_ENTER;
        case ' ':  return KEY_SPACE;
        case '\t': return KEY_TAB;
        case 27: {
            int b2 = read_byte_after(20);
            if (b2 < 0) return KEY_ESC;
            if (b2 == '[') {
                int b3 = read_byte();
                switch (b3) {
                    case 'A': return KEY_UP;
                    case 'B': return KEY_DOWN;
                    case 'C': return KEY_RIGHT;
                    case 'D': return KEY_LEFT;
                    default:
                        /* 形如 ESC[1~ ESC[2~ 等：吞掉结尾的 '~' */
                        if (b3 >= '0' && b3 <= '9') read_byte();
                        return KEY_OTHER;
                }
            }
            return KEY_OTHER;
        }
        case 'q': case 'Q': return KEY_Q;
        case 'w': case 'W': return KEY_W;
        case 's': case 'S': return KEY_S;
        case 'a': case 'A': return KEY_A;
        case 'd': case 'D': return KEY_D;
        case 'p': case 'P': return KEY_P;
        case 'f': case 'F': return KEY_F;
        case 'r': case 'R': return KEY_R;
        case '1': case '!': return KEY_1;
        case '2': case '@': return KEY_2;
        case '3': case '#': return KEY_3;
        case '4': case '$': return KEY_4;
        case '5': case '%': return KEY_5;
        case '6': case '^': return KEY_6;
        case '7': case '&': return KEY_7;
        case '8': case '*': return KEY_8;
        case '9': case '(': return KEY_9;
        case '0': case ')': return KEY_0;
        default:  return KEY_OTHER;
    }
}

/* 一些游戏需要短暂等待输入以显示结算画面，这里直接阻塞读取一个键 */
Key tui_wait_any(void) {
    for (;;) {
        Key k = tui_getch_timeout(10000);
        if (k != KEY_NONE) return k;
    }
}
