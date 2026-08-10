#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "games.h"
#include "tui.h"

static int b[9];               /* 0=空 1=X(玩家) 2=O(电脑) */
static int cur_r, cur_c;       /* 光标 */
static int win_line[3];        /* 获胜三格，-1 表示未获胜 */
static int result;             /* -1 未定 0 平 1 X赢 2 O赢 */
static bool thinking;          /* 电脑思考中 */

static const int LINES[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
    {0, 4, 8}, {2, 4, 6},
};

static void reset(void) {
    memset(b, 0, sizeof b);
    cur_r = cur_c = 0;
    win_line[0] = win_line[1] = win_line[2] = -1;
    result = -1;
    thinking = false;
}

static void check_result(void) {
    win_line[0] = win_line[1] = win_line[2] = -1;
    for (int i = 0; i < 8; i++) {
        int a = LINES[i][0], x = LINES[i][1], d = LINES[i][2];
        if (b[a] != 0 && b[a] == b[x] && b[x] == b[d]) {
            win_line[0] = a; win_line[1] = x; win_line[2] = d;
            result = b[a];
            return;
        }
    }
    bool empty = false;
    for (int i = 0; i < 9; i++) if (b[i] == 0) { empty = true; break; }
    result = empty ? -1 : 0;
}

static void cpu_move(void) {
    int mv = -1;
    /* 1. 自己能赢 */
    for (int m = 0; m < 9 && mv < 0; m++)
        if (b[m] == 0) {
            b[m] = 2;
            check_result();
            if (result == 2) mv = m;
            result = -1;
            win_line[0] = win_line[1] = win_line[2] = -1;
            b[m] = 0;
        }
    /* 2. 阻止玩家赢 */
    for (int m = 0; m < 9 && mv < 0; m++)
        if (b[m] == 0) {
            b[m] = 1;
            check_result();
            if (result == 1) mv = m;
            result = -1;
            win_line[0] = win_line[1] = win_line[2] = -1;
            b[m] = 0;
        }
    /* 3. 中心 */
    if (mv < 0 && b[4] == 0) mv = 4;
    /* 4. 随机角 */
    if (mv < 0) {
        int corners[4] = {0, 2, 6, 8};
        for (int i = 3; i > 0; i--) {
            int j = rand() % (i + 1);
            int t = corners[i]; corners[i] = corners[j]; corners[j] = t;
        }
        for (int i = 0; i < 4 && mv < 0; i++)
            if (b[corners[i]] == 0) mv = corners[i];
    }
    /* 5. 任意空位 */
    if (mv < 0) {
        int empties[9], n = 0;
        for (int i = 0; i < 9; i++) if (b[i] == 0) empties[n++] = i;
        if (n > 0) mv = empties[rand() % n];
    }
    if (mv >= 0) {
        b[mv] = 2;
        check_result();
    }
}

static void put_sym_abs(int r, int c, int style, const char *s) {
    tui_set(style);
    tui_text(r, c, s);
}

static void render(int x0, int y0) {
    /* 网格（内部 3 行 3 列，格宽 3 高 2，内容在 2r+1, 4c+1 相对偏移） */
    tui_set(TUI_GREEN);
    tui_box(y0, x0, y0 + 6, x0 + 12);
    tui_text(y0 + 2, x0, "╠═══╬═══╬═══╣");
    tui_text(y0 + 4, x0, "╠═══╬═══╬═══╣");

    for (int pos = 0; pos < 9; pos++) {
        int r = y0 + 1 + (pos / 3) * 2;
        int c = x0 + 1 + (pos % 3) * 4;
        bool hl = win_line[0] >= 0 && (win_line[0] == pos || win_line[1] == pos || win_line[2] == pos);
        bool cursor = (pos / 3 == cur_r && pos % 3 == cur_c) && result < 0;
        /* 先清格内，避免光标移动后旧位置残留 */
        tui_set(TUI_GREEN);
        tui_fill(r, c, 3, ' ');
        if (b[pos] == 1)
            put_sym_abs(r, c, hl ? TUI_INVERSE : TUI_GREEN_BRIGHT, "X");
        else if (b[pos] == 2)
            put_sym_abs(r, c, hl ? TUI_INVERSE : TUI_YELLOW, "O");
        else if (cursor)
            put_sym_abs(r, c, TUI_INVERSE, "·");
    }

    /* 状态行（先清两行，避免切换时残留） */
    tui_set(TUI_RESET);
    tui_fill(y0 + 8, x0, 36, ' ');
    tui_fill(y0 + 9, x0, 36, ' ');
    tui_set(TUI_GREEN_DIM);
    if (result == 1)      tui_put_safe(y0 + 8, x0, "你赢了！  [R] 再来一局  [Q] 返回");
    else if (result == 2) tui_put_safe(y0 + 8, x0, "电脑赢了  [R] 再来一局  [Q] 返回");
    else if (result == 0) tui_put_safe(y0 + 8, x0, "平局！    [R] 再来一局  [Q] 返回");
    else if (thinking)    tui_put_safe(y0 + 8, x0, "电脑思考中……");
    else {
        tui_put_safe(y0 + 8, x0, "你是 X，电脑是 O");
        tui_put_safe(y0 + 9, x0, "[方向键] 移动光标  [Enter] 落子  [Q] 退出");
    }
    tui_set(TUI_RESET);
    fflush(stdout);
}

int game_tictactoe_run(void) {
    srand((unsigned)(time(NULL) ^ (long)tui_pid() << 2));
    reset();
    int x0 = tui_center_x(14);
    int y0 = tui_center_row(12);

    /* 标题 */
    tui_set(TUI_GREEN_BRIGHT);
    tui_text(y0 - 2, x0, "TIC TAC TOE");

    for (;;) {
        render(x0, y0);
        if (result >= 0) {
            Key k = tui_getch_timeout(2000);
            if (k == KEY_R) { reset(); continue; }
            return 0;
        }
        if (thinking) {
            tui_sleep(350);
            thinking = false;
            continue;
        }
        Key k = tui_getch_timeout(500);
        switch (k) {
            case KEY_UP:    cur_r = (cur_r + 2) % 3; break;
            case KEY_DOWN:  cur_r = (cur_r + 1) % 3; break;
            case KEY_LEFT:  cur_c = (cur_c + 2) % 3; break;
            case KEY_RIGHT: cur_c = (cur_c + 1) % 3; break;
            case KEY_ENTER: case KEY_SPACE: {
                int pos = cur_r * 3 + cur_c;
                if (b[pos] != 0) break;
                b[pos] = 1;
                check_result();
                if (result >= 0) break;
                thinking = true;
                cpu_move();
                break;
            }
            case KEY_R: reset(); break;
            case KEY_Q: case KEY_ESC: return 0;
            default: break;
        }
    }
}
