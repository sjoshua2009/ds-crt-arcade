#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "games.h"
#include "tui.h"

#define N 4

static int b[N][N];
static int score;
static bool won;
static bool no_random;   /* 测试用：移动后不生成随机块 */

static void rot90cw(void) {
    int t[N][N];
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            t[c][N - 1 - r] = b[r][c];
    memcpy(b, t, sizeof b);
}

static void mirror_x(void) {
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N / 2; c++) {
            int t = b[r][c];
            b[r][c] = b[r][N - 1 - c];
            b[r][N - 1 - c] = t;
        }
}

/* 一行向左滑动合并，返回本行得分增量 */
static int slide_row(int row[N]) {
    int out[N] = {0};
    bool merged[N] = {false};
    int pos = 0, gained = 0;
    for (int c = 0; c < N; c++) {
        int v = row[c];
        if (!v) continue;
        if (pos > 0 && out[pos - 1] == v && !merged[pos - 1]) {
            out[pos - 1] = v * 2;
            merged[pos - 1] = true;
            gained += v * 2;
            if (v * 2 >= 2048) won = true;
        } else {
            out[pos++] = v;
        }
    }
    memcpy(row, out, sizeof out);
    return gained;
}

static void add_random(void) {
    int cells = 0;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            if (b[r][c] == 0) cells++;
    if (cells == 0) return;
    int idx = rand() % cells;
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            if (b[r][c] == 0 && idx-- == 0) {
                b[r][c] = (rand() % 10 == 0) ? 4 : 2;
                return;
            }
}

static bool can_move(void) {
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++) {
            if (b[r][c] == 0) return true;
            if (r + 1 < N && b[r][c] == b[r + 1][c]) return true;
            if (c + 1 < N && b[r][c] == b[r][c + 1]) return true;
        }
    return false;
}

/* 0=左 1=右 2=上 3=下；发生移动则生成新块并返回 true */
static bool move_dir(int dir) {
    int before[N][N];
    memcpy(before, b, sizeof b);
    if (dir == 1) mirror_x();
    else if (dir == 2) { rot90cw(); rot90cw(); rot90cw(); }
    else if (dir == 3) rot90cw();

    int gained = 0;
    for (int r = 0; r < N; r++) gained += slide_row(b[r]);

    if (dir == 1) mirror_x();
    else if (dir == 2) rot90cw();
    else if (dir == 3) { rot90cw(); rot90cw(); rot90cw(); }

    if (memcmp(before, b, sizeof b) == 0) return false;
    score += gained;
    if (!no_random) add_random();
    return true;
}

static void reset(void) {
    memset(b, 0, sizeof b);
    score = 0;
    won = false;
    add_random();
    add_random();
}

static int style_for(int v) {
    if (v == 0)    return TUI_GREEN_DIM;
    if (v <= 4)    return TUI_GREEN;
    if (v <= 16)   return TUI_GREEN_BRIGHT;
    if (v <= 64)   return TUI_YELLOW;
    if (v <= 256)  return TUI_INVERSE;
    if (v <= 1024) return TUI_RED;
    return TUI_INVERSE;
}

static void put_tile(int r, int c, int x0, int y0) {
    int v = b[r][c];
    tui_set(style_for(v));
    char buf[8];
    if (v == 0) buf[0] = '\0';
    else snprintf(buf, sizeof buf, "%d", v);
    int len = (int)strlen(buf);
    int pad = (7 - len) / 2;
    tui_printf(y0 + r, x0 + c * 7 + 1, "%*s%s%*s", pad, "", buf, 7 - len - pad, "");
}

static void render(int x0, int y0) {
    tui_set(TUI_GREEN);
    tui_box(y0, x0, y0 + 2 * N + 1, x0 + 7 * N + 1);
    for (int r = 1; r <= N - 1; r++) {
        char line[128] = "";
        strcat(line, "╠");
        for (int c = 0; c < N; c++) {
            strcat(line, "═══════");
            strcat(line, (c == N - 1) ? "╣" : "╬");
        }
        tui_text(y0 + 2 * r, x0, line);
    }
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            put_tile(r, c, x0, y0 + 1 + 2 * r);

    tui_set(TUI_GREEN_BRIGHT);
    tui_text(y0 - 2, x0, "2048");
    tui_set(TUI_YELLOW);
    char buf[40];
    snprintf(buf, sizeof(buf), "SCORE  %07d", score);
    tui_text(y0 - 2, x0 + 8, buf);
    tui_set(TUI_GREEN_DIM);
    tui_put_safe(y0 + 2 * N + 2, x0, "[方向键/WASD] 移动   [R] 重开   [Q] 退出");
    tui_set(TUI_RESET);
    fflush(stdout);
}

static void show_banner(int x0, int y0, const char *msg, int style, bool end) {
    tui_set(style);
    tui_text(y0 + N, x0 + 3, msg);
    if (end) {
        tui_set(TUI_YELLOW);
        char buf[48];
        snprintf(buf, sizeof(buf), "FINAL  %07d", score);
        tui_text(y0 + N + 2, x0 + 3, buf);
        tui_set(TUI_GREEN_DIM);
        tui_text(y0 + N + 4, x0 - 4, "[R] 再来一局   [任意键] 返回大厅   [Q] 退出");
    }
    tui_set(TUI_RESET);
    fflush(stdout);
}

int game_2048_run(void) {
    srand((unsigned)(time(NULL) ^ (long)tui_pid() << 4));
    int x0 = tui_center_x(7 * N + 2);
    int y0 = tui_center_row(2 * N + 6);

restart:
    reset();
    for (;;) {
        render(x0, y0);
        if (won) {
            show_banner(x0, y0, "★★  YOU WIN! 2048 ★★", TUI_GREEN_BRIGHT, false);
        }
        Key k = tui_getch_timeout(2000);
        switch (k) {
            case KEY_LEFT:  case KEY_A: move_dir(0); break;
            case KEY_RIGHT: case KEY_D: move_dir(1); break;
            case KEY_UP:    case KEY_W: move_dir(2); break;
            case KEY_DOWN:  case KEY_S: move_dir(3); break;
            case KEY_R: reset(); break;
            case KEY_Q: case KEY_ESC: return 0;
            default: break;
        }
        if (!can_move()) break;
    }

    /* 死局 */
    render(x0, y0);
    show_banner(x0, y0, " GAME OVER · 无路可走 ", TUI_RED, true);
    for (;;) {
        Key k = tui_getch_timeout(2000);
        if (k == KEY_R) goto restart;
        return 0;
    }
}
