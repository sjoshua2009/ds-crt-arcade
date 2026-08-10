#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "games.h"
#include "tui.h"

game_t g_games[] = {
    {"01", "SNAKE",       "贪吃蛇",   "[经典]", "吃掉食物不断变长",  game_snake_run},
    {"02", "TETRIS",      "俄罗斯方块", "[经典]", "行消消乐挑战极限",   game_tetris_run},
    {"03", "2048",        "2048",     "[益智]", "合成 2048 大数字",   game_2048_run},
    {"04", "TIC-TAC-TOE", "井字棋",   "[对战]", "和电脑下三子棋",     game_tictactoe_run},
    {"05", "MINESWEEPER", "扫雷",     "[逻辑]", "推理排除所有地雷",   game_minesweeper_run},
    {"06", "PONG",        "乒乓球",   "[街机]", "左右互搏经典乒乓",   game_pong_run},
};
int g_game_count = (int)(sizeof(g_games) / sizeof(g_games[0]));

#define LB_W_MAX 66
#define LB_H_MAX 20

static const char *const TITLE[] = {
    " ██████╗  █████╗ ███╗   ███╗███████╗     ██╗      ██████╗ ██████╗ ██████╗ ██╗   ██╗",
    "██╔════╝ ██╔══██╗████╗ ████║██╔════╝     ██║     ██╔═══██╗██╔═══██╗██╔═══██╗╚██╗ ██╔╝",
    "██║  ███╗███████║██╔████╔██║█████╗       ██║     ██║   ██║██║   ██║██║   ██║ ╚████╔╝ ",
    "██║   ██║██╔══██║██║╚██╔╝██║██╔══╝       ██║     ██║   ██║██║   ██║██║   ██║  ╚██╔╝  ",
    "╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗     ███████╗╚██████╔╝╚██████╔╝╚██████╔╝   ██║   ",
    " ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝     ╚══════╝ ╚═════╝  ╚═════╝  ╚═════╝    ╚═╝   ",
};

static void draw_menu(int sel, int x, int y, int w, int max_rows) {
    bool show_tag = w >= 46;
    if (max_rows > g_game_count) max_rows = g_game_count;
    for (int i = 0; i < max_rows; i++) {
        int r = y + i;
        if (i == sel) {
            tui_set(TUI_INVERSE);
            tui_text(r, x + 2, "▶ ");
            char line[48];
            snprintf(line, sizeof(line), "%s  %-14s %-10s",
                     g_games[i].id, g_games[i].name, g_games[i].zh);
            tui_put_safe(r, x + 5, line);
            tui_set(TUI_RESET);
        } else {
            tui_set(TUI_GREEN_BRIGHT);
            tui_text(r, x + 2, "  ");
            tui_put_safe(r, x + 5, g_games[i].id);
            tui_set(TUI_GREEN);
            tui_put_safe(r, x + 8, g_games[i].name);
            tui_set(TUI_GREEN_DIM);
            tui_put_safe(r, x + 23, g_games[i].zh);
            if (show_tag) {
                tui_set(TUI_YELLOW);
                tui_put_safe(r, x + 34, g_games[i].tag);
            }
        }
    }
}

static void draw_lobby(int sel) {
    int cols = tui_term_cols();
    int rows = tui_term_rows();
    /* 画布动态缩放：完整 66x20，放不下则贴终端尺寸收缩 */
    int w = cols - 2 < LB_W_MAX ? cols - 2 : LB_W_MAX;
    int h = rows - 2 < LB_H_MAX ? rows - 2 : LB_H_MAX;
    int x = (cols - w) / 2;
    if (x < 1) x = 1;
    int y = (rows - h) / 2;
    if (y < 1) y = 1;

    tui_clear();
    tui_box(y, x, y + h - 1, x + w - 1);

    /* 标题：空间足够用块字，否则单行大字 */
    int yp = y + 1;
    if (w >= 66 && h >= 19) {
        tui_set(TUI_GREEN_BRIGHT);
        for (int i = 0; i < 6; i++)
            tui_text(yp + i, x + 2, TITLE[i]);
        yp += 7;
    } else {
        tui_set(TUI_GREEN_BRIGHT);
        tui_text(yp, x + 2, " G A M E   L O B B Y ");
        yp += 2;
    }
    tui_set(TUI_GREEN_DIM);
    tui_put_safe(yp, x + 2, "v1.0 · 6 GAMES · CRT EDITION");
    yp++;
    tui_hline(yp, x + 2, x + w - 3, TUI_GREEN_DIM);
    yp++;

    draw_menu(sel, x, yp, w, h - 4);

    tui_hline(y + h - 3, x + 2, x + w - 3, TUI_GREEN_DIM);
    tui_set(TUI_GREEN);
    if (w >= 50)
        tui_put_safe(y + h - 2, x + 2, "[↑↓/WS] 选择  [Enter] 进入  [1-6] 快捷");
    else
        tui_put_safe(y + h - 2, x + 2, "[↑↓/WS] 选择  [1-6] 快捷");
    tui_set(TUI_YELLOW);
    tui_put_safe(y + h - 2, x + w - 9, "[Q] 退出");
    tui_set(TUI_RESET);
}

static int run_game(int idx) {
    tui_clear();
    int r = g_games[idx].fn();
    tui_clear();
    return r;
}

int main(void) {
    tui_init();
    atexit(tui_restore);

    int sel = 0;
    for (;;) {
        draw_lobby(sel);
        Key k = tui_getch_timeout(200);
        switch (k) {
            case KEY_UP:    case KEY_W: sel = (sel + g_game_count - 1) % g_game_count; break;
            case KEY_DOWN:  case KEY_S: sel = (sel + 1) % g_game_count; break;
            case KEY_ENTER: case KEY_SPACE: run_game(sel); break;
            case KEY_1: run_game(0); break;
            case KEY_2: run_game(1); break;
            case KEY_3: run_game(2); break;
            case KEY_4: run_game(3); break;
            case KEY_5: run_game(4); break;
            case KEY_6: run_game(5); break;
            case KEY_Q: goto quit;
            default: break;
        }
    }

quit:
    tui_clear();
    tui_set(TUI_GREEN_BRIGHT);
    tui_move(tui_center_row(1), tui_center_x(20));
    printf("BYE · 欢迎再来游戏大厅\n");
    tui_set(TUI_RESET);
    return 0;
}
