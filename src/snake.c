#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "games.h"
#include "tui.h"

#define W 26
#define H_MAX 14

typedef struct { int x, y; } Pt;

static int h = H_MAX;           /* 运行时高度：按终端自动缩小 */
static Pt snake[W * H_MAX];
static int len;
static int dir;      /* 0=上 1=下 2=左 3=右 */
static int next_dir;
static Pt food;
static int score;
static int delay_ms;

static void place_food(void) {
    int cells = W * h - len;
    if (cells <= 0) return;
    int idx = rand() % cells;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < W; x++) {
            bool occ = false;
            for (int i = 0; i < len; i++)
                if (snake[i].x == x && snake[i].y == y) { occ = true; break; }
            if (!occ) {
                if (idx-- == 0) { food.x = x; food.y = y; return; }
            }
        }
    }
}

static void init_game(void) {
    len = 3;
    for (int i = 0; i < len; i++) { snake[i].x = 5 - i; snake[i].y = h / 2; }
    dir = next_dir = 3;
    score = 0;
    delay_ms = 120;
    place_food();
}

static void draw(int x0, int y0) {
    tui_set(TUI_GREEN);
    tui_box(y0, x0, y0 + h + 1, x0 + W + 1);

    /* 先清空盒内区域，避免蛇尾移动后旧位置残留 */
    tui_set(TUI_RESET);
    for (int r = 0; r < h; r++)
        tui_fill(y0 + 1 + r, x0 + 1, W, ' ');

    tui_set(TUI_YELLOW);
    tui_text(y0 + 1 + food.y, x0 + 1 + food.x, "◈");

    for (int i = 0; i < len; i++) {
        tui_set(i == 0 ? TUI_INVERSE : TUI_GREEN_BRIGHT);
        tui_text(y0 + 1 + snake[i].y, x0 + 1 + snake[i].x, "█");
    }

    tui_set(TUI_GREEN_DIM);
    char buf[80];
    snprintf(buf, sizeof(buf), "SCORE %05d  LEN %02d  SPD x%d  [方向键/WASD] [P]暂停 [Q]退出",
             score, len, 1200 / delay_ms);
    tui_put_safe(y0 + h + 2, x0, buf);
    tui_set(TUI_RESET);
    fflush(stdout);
}

static void draw_pause(int x0, int y0) {
    tui_set(TUI_YELLOW);
    tui_text(y0 + 1, x0 + W / 2 - 4, "╔══════════╗");
    tui_text(y0 + 2, x0 + W / 2 - 4, "║  PAUSED  ║");
    tui_text(y0 + 3, x0 + W / 2 - 4, "╚══════════╝");
    tui_set(TUI_RESET);
    fflush(stdout);
}

static void show_end(int x0, int y0, bool won) {
    tui_set(won ? TUI_GREEN_BRIGHT : TUI_RED);
    tui_text(y0 + h / 2 - 1, x0 + W / 2 - 9, won ? "   ★ YOU WIN ★   " : "   G A M E  O V E R   ");
    tui_set(TUI_YELLOW);
    char buf[64];
    snprintf(buf, sizeof(buf), "FINAL SCORE  %05d", score);
    tui_text(y0 + h / 2 + 1, x0 + W / 2 - 7, buf);
    tui_set(TUI_GREEN_DIM);
    tui_text(y0 + h / 2 + 3, x0 + W / 2 - 11, "[任意键] 返回大厅   [Q] 直接退出");
    tui_set(TUI_RESET);
    fflush(stdout);
}

int game_snake_run(void) {
    srand((unsigned)(time(NULL) ^ (long)tui_pid() << 8));
    /* 高度随终端自动收缩：先整体缩小，再按剩余空间压缩 */
    h = H_MAX;
    int rows = tui_term_rows();
    if (rows < h + 5) h = 12;
    if (rows < h + 5) h = rows - 5;
    if (h < 8) h = 8;
    init_game();
    int x0 = tui_center_x(W + 2);
    int y0 = tui_center_row(h + 5);
    bool over = false, won = false, paused = false;

    while (!over && !won) {
        Key k;
        while ((k = tui_getch_timeout(0)) != KEY_NONE) {
            switch (k) {
                case KEY_UP:    case KEY_W: if (dir != 1) next_dir = 0; break;
                case KEY_DOWN:  case KEY_S: if (dir != 0) next_dir = 1; break;
                case KEY_LEFT:  case KEY_A: if (dir != 3) next_dir = 2; break;
                case KEY_RIGHT: case KEY_D: if (dir != 2) next_dir = 3; break;
                case KEY_P: paused = !paused; break;
                case KEY_Q: case KEY_ESC: return 0;
                default: break;
            }
        }
        if (paused) {
            draw(x0, y0);
            draw_pause(x0, y0);
            tui_sleep(60);
            continue;
        }

        dir = next_dir;
        Pt nh = snake[0];
        if (dir == 0) nh.y--;
        else if (dir == 1) nh.y++;
        else if (dir == 2) nh.x--;
        else nh.x++;

        if (nh.x < 0 || nh.x >= W || nh.y < 0 || nh.y >= h) { over = true; break; }

        bool ate = (nh.x == food.x && nh.y == food.y);
        if (ate) {
            len++;
            score += 10;
            if (delay_ms > 40) delay_ms -= 3;
            memmove(snake + 1, snake, (size_t)(len - 1) * sizeof(Pt));
            snake[0] = nh;
            if (len >= W * h) { won = true; break; }
            place_food();
        } else {
            for (int i = 0; i < len; i++)
                if (snake[i].x == nh.x && snake[i].y == nh.y) { over = true; break; }
            if (over) break;
            memmove(snake + 1, snake, (size_t)(len - 1) * sizeof(Pt));
            snake[0] = nh;
        }
        draw(x0, y0);
        tui_sleep(delay_ms);
    }

    draw(x0, y0);
    show_end(x0, y0, won);
    for (;;) {
        Key k = tui_getch_timeout(1000);
        if (k == KEY_Q || k == KEY_ESC) return 0;
        if (k != KEY_NONE) return 0;
    }
}
