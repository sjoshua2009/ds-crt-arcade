#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "games.h"
#include "tui.h"

#define MS_N 9
#define MS_MINES 10

static bool mine[MS_N][MS_N];
static char state[MS_N][MS_N];   /* 0=未开 1=已开 2=旗 3=问号 */
static int flags;
static int opened;
static int cur_r, cur_c;
static bool over, won, first;

static void reset(void) {
    memset(mine, 0, sizeof mine);
    memset(state, 0, sizeof state);
    flags = opened = 0;
    cur_r = cur_c = 0;
    over = won = false;
    first = true;
}

static void place_mines(int safe_r, int safe_c) {
    int placed = 0;
    while (placed < MS_MINES) {
        int r = rand() % MS_N, c = rand() % MS_N;
        if (mine[r][c]) continue;
        if ((r == safe_r || r == safe_r + 1 || r == safe_r - 1) &&
            (c == safe_c || c == safe_c + 1 || c == safe_c - 1))
            continue;   /* 首次点击的 3x3 安全区不放雷 */
        mine[r][c] = true;
        placed++;
    }
}

static int count_mines(int r, int c) {
    int n = 0;
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            int rr = r + dr, cc = c + dc;
            if (rr >= 0 && rr < MS_N && cc >= 0 && cc < MS_N && mine[rr][cc]) n++;
        }
    return n;
}

static void flood_open(int r, int c) {
    if (r < 0 || r >= MS_N || c < 0 || c >= MS_N) return;
    if (state[r][c] != 0) return;   /* 已开/旗/问号都跳过 */
    if (mine[r][c]) return;
    state[r][c] = 1;
    opened++;
    if (count_mines(r, c) == 0)
        for (int dr = -1; dr <= 1; dr++)
            for (int dc = -1; dc <= 1; dc++)
                if (dr || dc) flood_open(r + dr, c + dc);
}

static void reveal_all_mines(int hit_r, int hit_c) {
    for (int r = 0; r < MS_N; r++)
        for (int c = 0; c < MS_N; c++)
            if (mine[r][c]) state[r][c] = (r == hit_r && c == hit_c) ? 4 : 5;
            /* 4=踩中的雷(红反白) 5=普通雷(红) */
}

static void open_cell(int r, int c) {
    if (state[r][c] == 1 || state[r][c] == 2) return;
    if (first) {
        place_mines(r, c);
        first = false;
    }
    if (mine[r][c]) {
        over = true;
        reveal_all_mines(r, c);
        return;
    }
    state[r][c] = 1;
    opened++;
    if (count_mines(r, c) == 0)
        for (int dr = -1; dr <= 1; dr++)
            for (int dc = -1; dc <= 1; dc++)
                if (dr || dc) flood_open(r + dr, c + dc);
    if (opened >= MS_N * MS_N - MS_MINES) won = true;
}

static void cycle_flag(int r, int c) {
    if (state[r][c] == 1) return;
    if (state[r][c] == 0) { state[r][c] = 2; flags++; }
    else if (state[r][c] == 2) { state[r][c] = 3; flags--; }
    else if (state[r][c] == 3) state[r][c] = 0;
}

static int num_style(int v) {
    switch (v) {
        case 1: return TUI_GREEN;
        case 2: return TUI_GREEN_BRIGHT;
        case 3: return TUI_YELLOW;
        case 4: return TUI_INVERSE;
        case 5: return TUI_RED;
        default: return TUI_GREEN_BRIGHT;
    }
}

static void render(int x0, int y0) {
    tui_set(TUI_GREEN);
    tui_box(y0, x0, y0 + MS_N + 1, x0 + MS_N * 3 + 1);

    for (int r = 0; r < MS_N; r++) {
        for (int c = 0; c < MS_N; c++) {
            int st = state[r][c];
            int style = TUI_GREEN_DIM;
            const char *s;
            if (st == 1) {
                int n = count_mines(r, c);
                if (n == 0) { s = "·"; style = TUI_GREEN_DIM; }
                else { char *tmp = (char[]){'0' + n, 0}; s = tmp; style = num_style(n); }
            } else if (st == 2) { s = "⚑"; style = TUI_YELLOW; }
            else if (st == 3)   { s = "?"; style = TUI_GREEN_BRIGHT; }
            else if (st == 4)   { s = "*"; style = TUI_INVERSE; }
            else if (st == 5)   { s = "*"; style = TUI_RED; }
            else { s = "▒"; style = TUI_GREEN_DIM; }

            bool cursor = (r == cur_r && c == cur_c) && !over && !won;
            if (cursor) style = TUI_INVERSE;
            tui_set(style);
            tui_printf(y0 + 1 + r, x0 + 1 + c * 3, " %s ", s);
        }
    }

    char buf[64];
    tui_set(TUI_GREEN_BRIGHT);
    tui_text(y0 - 2, x0, "MINESWEEPER");
    tui_set(TUI_YELLOW);
    snprintf(buf, sizeof buf, "雷: %02d", MS_MINES - flags);
    tui_text(y0 - 2, x0 + 16, buf);
    tui_set(TUI_GREEN);
    snprintf(buf, sizeof buf, "已开: %02d / %d", opened, MS_N * MS_N - MS_MINES);
    tui_text(y0 - 2, x0 + 27, buf);

    /* 状态行（先清两行，避免正常/结束切换时残留） */
    tui_set(TUI_RESET);
    tui_fill(y0 + MS_N + 2, x0, 44, ' ');
    tui_fill(y0 + MS_N + 3, x0, 44, ' ');
    tui_set(TUI_GREEN_DIM);
    if (over)
        tui_put_safe(y0 + MS_N + 2, x0, "踩雷了！ [R] 再来一局  [Q] 返回");
    else if (won)
        tui_put_safe(y0 + MS_N + 2, x0, "扫雷成功！ [R] 再来一局  [Q] 返回");
    else {
        tui_put_safe(y0 + MS_N + 2, x0, "[方向键] 移动  [Enter] 揭开  [F] 插旗/问号");
        tui_put_safe(y0 + MS_N + 3, x0, "[R] 重开  [Q] 退出");
    }
    tui_set(TUI_RESET);
    fflush(stdout);
}

int game_minesweeper_run(void) {
    srand((unsigned)(time(NULL) ^ (long)tui_pid() << 1));
    reset();
    int x0 = tui_center_x(MS_N * 3 + 2);
    int y0 = tui_center_row(MS_N + 6);

    for (;;) {
        render(x0, y0);
        if (over || won) {
            Key k = tui_getch_timeout(2000);
            if (k == KEY_R) { reset(); continue; }
            return 0;
        }
        Key k = tui_getch_timeout(300);
        switch (k) {
            case KEY_UP:    cur_r = (cur_r + MS_N - 1) % MS_N; break;
            case KEY_DOWN:  cur_r = (cur_r + 1) % MS_N; break;
            case KEY_LEFT:  cur_c = (cur_c + MS_N - 1) % MS_N; break;
            case KEY_RIGHT: cur_c = (cur_c + 1) % MS_N; break;
            case KEY_ENTER: case KEY_SPACE: open_cell(cur_r, cur_c); break;
            case KEY_F: cycle_flag(cur_r, cur_c); break;
            case KEY_R: reset(); break;
            case KEY_Q: case KEY_ESC: return 0;
            default: break;
        }
    }
}
