#include "tui.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <wchar.h>
#include <windows.h>
#include <process.h>
#include <curses.h>             /* PDCurses（Windows 构建前由 CI 拉取到 deps/pdcurses） */

/* PDCurses 的 KEY_UP/DOWN/LEFT/RIGHT/ENTER 是宏，与 tui.h 中的枚举重名。
 * 用 enum 在取消宏定义之前捕获 PDCurses 的按键码（getch() 返回这些值），
 * 再 #undef，使本文件后续代码里 KEY_* 恢复为 tui.h 的枚举语义。
 * 注意不能用 #define 另存，因为宏在使用点才展开，届时 KEY_* 已被取消定义。 */
enum {
    TUI_PDC_KEY_UP    = KEY_UP,
    TUI_PDC_KEY_DOWN  = KEY_DOWN,
    TUI_PDC_KEY_LEFT  = KEY_LEFT,
    TUI_PDC_KEY_RIGHT = KEY_RIGHT,
    TUI_PDC_KEY_ENTER = KEY_ENTER
};
#undef KEY_UP
#undef KEY_DOWN
#undef KEY_LEFT
#undef KEY_RIGHT
#undef KEY_ENTER
#else
#include <poll.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#endif

#ifndef _WIN32
static struct termios g_orig;
static bool g_raw = false;
#endif

void tui_init(void) {
#ifdef _WIN32
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN,  COLOR_BLACK);  /* 暗绿 */
        init_pair(2, COLOR_GREEN,  COLOR_BLACK);  /* 常规绿 */
        init_pair(3, COLOR_GREEN,  COLOR_BLACK);  /* 亮绿（配 A_BOLD） */
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);  /* 黄 */
        init_pair(5, COLOR_RED,    COLOR_BLACK);  /* 红 */
        init_pair(6, COLOR_BLACK,  COLOR_GREEN);  /* 反白：黑字亮绿底 */
    }
#else
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
#endif
}

void tui_restore(void) {
#ifdef _WIN32
    curs_set(1);
    endwin();
#else
    if (g_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig);
        g_raw = false;
    }
    printf("\x1b[?25h\x1b[0m");
    fflush(stdout);
#endif
}

void tui_clear(void) {
#ifdef _WIN32
    clear();
    refresh();
#else
    printf("\x1b[2J\x1b[H");
    fflush(stdout);
#endif
}

void tui_move(int row, int col) {
#ifdef _WIN32
    move(row - 1, col - 1);
#else
    printf("\x1b[%d;%dH", row, col);
#endif
}

void tui_set(int style) {
#ifdef _WIN32
    int p;
    for (p = 1; p <= 6; p++) attroff(COLOR_PAIR(p));
    attroff(A_BOLD | A_REVERSE | A_UNDERLINE);
    switch (style) {
        case TUI_GREEN_DIM:    attron(COLOR_PAIR(1)); break;
        case TUI_GREEN:        attron(COLOR_PAIR(2)); break;
        case TUI_GREEN_BRIGHT: attron(COLOR_PAIR(3) | A_BOLD); break;
        case TUI_YELLOW:       attron(COLOR_PAIR(4) | A_BOLD); break;
        case TUI_RED:          attron(COLOR_PAIR(5) | A_BOLD); break;
        case TUI_INVERSE:      attron(COLOR_PAIR(6)); break;
        default: break;                                    /* TUI_RESET */
    }
#else
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
#endif
}

#ifndef _WIN32
static void put_utf8(const char *s) {
    fputs(s, stdout);
}
#endif

void tui_box(int r0, int c0, int r1, int c1) {
    tui_set(TUI_GREEN);
#ifdef _WIN32
    mvaddwstr(r0 - 1, c0 - 1, L"╔");
    for (int c = c0 + 1; c < c1; c++) mvaddwstr(r0 - 1, c - 1, L"═");
    mvaddwstr(r0 - 1, c1 - 1, L"╗");
    for (int r = r0 + 1; r < r1; r++) {
        mvaddwstr(r - 1, c0 - 1, L"║");
        mvaddwstr(r - 1, c1 - 1, L"║");
    }
    mvaddwstr(r1 - 1, c0 - 1, L"╚");
    for (int c = c0 + 1; c < c1; c++) mvaddwstr(r1 - 1, c - 1, L"═");
    mvaddwstr(r1 - 1, c1 - 1, L"╝");
    refresh();
#else
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
#endif
}

void tui_hline(int r, int c0, int c1, int style) {
    tui_set(style);
#ifdef _WIN32
    for (int c = c0; c <= c1; c++) mvaddwstr(r - 1, c - 1, L"─");
    refresh();
#else
    tui_move(r, c0);
    for (int c = c0; c <= c1; c++) put_utf8("─");
    fflush(stdout);
#endif
}

#ifdef _WIN32
/* UTF-8 字节串解码为 wchar_t 数组（PDCurses 宽字符输出用），返回字符数 */
static size_t utf8_to_wcs(const char *s, wchar_t *out, size_t cap) {
    size_t n = 0;
    const unsigned char *p = (const unsigned char *)s;
    while (*p && n + 1 < cap) {
        unsigned c = *p++;
        if (c < 0x80) { out[n++] = (wchar_t)c; continue; }
        int extra;
        unsigned cp;
        if ((c & 0xE0) == 0xC0)      { cp = c & 0x1F; extra = 1; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
        else { out[n++] = (wchar_t)'?'; continue; }
        int ok = 1;
        for (int i = 0; i < extra; i++) {
            if ((*p & 0xC0) != 0x80) { ok = 0; break; }
            cp = (cp << 6) | (*p++ & 0x3F);
        }
        out[n++] = ok ? (wchar_t)cp : (wchar_t)'?';
    }
    out[n] = 0;
    return n;
}

/* 粗略显示宽度：CJK 等宽字符占 2 列，其余 1 列 */
static int wch_w(int c) {
    if (c >= 0x1100 && (c <= 0x115F || c == 0x2329 || c == 0x232A ||
        (c >= 0x2E80 && c <= 0xA4CF && c != 0x303F) ||
        (c >= 0xAC00 && c <= 0xD7A3) || (c >= 0xF900 && c <= 0xFAFF) ||
        (c >= 0xFE30 && c <= 0xFE4F) || (c >= 0xFF00 && c <= 0xFF60) ||
        (c >= 0xFFE0 && c <= 0xFFE6) || (c >= 0x20000 && c <= 0x2FFFD)))
        return 2;
    return 1;
}
#endif

void tui_text(int r, int c, const char *s) {
#ifdef _WIN32
    wchar_t wb[512];
    utf8_to_wcs(s, wb, 512);
    mvaddwstr(r - 1, c - 1, wb);
    refresh();
#else
    tui_move(r, c);
    put_utf8(s);
    fflush(stdout);
#endif
}

void tui_printf(int r, int c, const char *fmt, ...) {
    va_list ap;
#ifdef _WIN32
    char buf[512];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    wchar_t wb[512];
    utf8_to_wcs(buf, wb, 512);
    mvaddwstr(r - 1, c - 1, wb);
    refresh();
#else
    tui_move(r, c);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
#endif
}

void tui_fill(int r, int c, int count, char ch) {
    if (count <= 0) return;
#ifdef _WIN32
    wchar_t wb[512];
    int i;
    for (i = 0; i < count && i < 511; i++) wb[i] = (wchar_t)(unsigned char)ch;
    wb[i] = 0;
    mvaddwstr(r - 1, c - 1, wb);
    refresh();
#else
    tui_move(r, c);
    for (int i = 0; i < count; i++) putchar(ch);
    fflush(stdout);
#endif
}

/* 定位输出，但截断到不超过终端右边界；不切断多字节 UTF-8 字符 */
void tui_put_safe(int r, int c, const char *s) {
    int max = tui_term_cols() - c;
    if (max <= 0) return;
#ifdef _WIN32
    wchar_t wb[256];
    size_t n = utf8_to_wcs(s, wb, 256);
    int cols = 0;
    size_t k;
    for (k = 0; k < n; k++) {
        int w = wch_w(wb[k]);
        if (cols + w > max) break;
        cols += w;
    }
    if (k == 0) return;
    wb[k] = 0;
    mvaddwstr(r - 1, c - 1, wb);
    refresh();
#else
    int n = 0;
    while (s[n] && n < max) n++;
    while (n > 0 && (s[n] & 0xC0) == 0x80) n--;
    if (n == 0) return;
    char buf[256];
    memcpy(buf, s, (size_t)n);
    buf[n] = '\0';
    tui_text(r, c, buf);
#endif
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
#ifdef _WIN32
    return COLS > 0 ? COLS : 80;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        return w.ws_col;
    return 80;
#endif
}

int tui_term_rows(void) {
    int v = term_override("TUI_ROWS");
    if (v) return v;
#ifdef _WIN32
    return LINES > 0 ? LINES : 24;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0)
        return w.ws_row;
    return 24;
#endif
}

int tui_center_x(int width) {
    int c = (tui_term_cols() - width) / 2;
    return c < 1 ? 1 : c;
}

int tui_center_row(int height) {
    int r = (tui_term_rows() - height) / 2;
    return r < 1 ? 1 : r;
}

#ifndef _WIN32
static int read_byte(void) {
    unsigned char b;
    int n = (int)read(STDIN_FILENO, &b, 1);
    return n == 1 ? (int)b : -1;
}

/* 尝试读取跟在 ESC 后面的一个字节，最多等待 ms 毫秒 */
static int read_byte_after(int ms) {
    struct pollfd p = {STDIN_FILENO, POLLIN, 0};
    if (poll(&p, 1, ms) <= 0) return -1;
    return read_byte();
}
#endif

Key tui_getch_timeout(int ms) {
#ifdef _WIN32
    timeout(ms);
    int b = getch();
    if (b == ERR) return KEY_NONE;
    switch (b) {
        case '\n': case '\r': return KEY_ENTER;
        case ' ':  return KEY_SPACE;
        case '\t': return KEY_TAB;
        case TUI_PDC_KEY_UP:    return KEY_UP;
        case TUI_PDC_KEY_DOWN:  return KEY_DOWN;
        case TUI_PDC_KEY_LEFT:  return KEY_LEFT;
        case TUI_PDC_KEY_RIGHT: return KEY_RIGHT;
        case TUI_PDC_KEY_ENTER: return KEY_ENTER;
        case 27: return KEY_ESC;
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
#else
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
#endif
}

/* 一些游戏需要短暂等待输入以显示结算画面，这里直接阻塞读取一个键 */
Key tui_wait_any(void) {
    for (;;) {
        Key k = tui_getch_timeout(10000);
        if (k != KEY_NONE) return k;
    }
}

void tui_sleep(int ms) {
    if (ms <= 0) return;
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

long tui_pid(void) {
#ifdef _WIN32
    return (long)_getpid();
#else
    return (long)getpid();
#endif
}
