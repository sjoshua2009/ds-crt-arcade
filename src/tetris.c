#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "games.h"
#include "tui.h"

#define BW 10
#define BH 20

static int board[BH][BW];          /* 0=空 1=已锁定方块 */
static uint32_t shape[4];          /* 当前方块 4x4 位图（每行低16位） */
static int shp_id;
static int next_id;
static int px, py;                 /* 当前方块左上角（网格坐标） */
static int score, lines, level;
static int drop_ms;
static bool over;

/* 每行低 16 位按 0x8000>>c 表示列 c（c=0 最左） */
static const uint32_t SHAPES[7][4] = {
    {0xF000, 0x0000, 0x0000, 0x0000},   /* I */
    {0xC000, 0xC000, 0x0000, 0x0000},   /* O */
    {0x4000, 0xE000, 0x0000, 0x0000},   /* T */
    {0x6000, 0xC000, 0x0000, 0x0000},   /* S */
    {0xC000, 0x6000, 0x0000, 0x0000},   /* Z */
    {0x8000, 0xE000, 0x0000, 0x0000},   /* J */
    {0x2000, 0xE000, 0x0000, 0x0000},   /* L */
};

static bool collide(int y, int x, const uint32_t *m) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (m[r] & (0x8000 >> c)) {
                int by = y + r, bx = x + c;
                if (bx < 0 || bx >= BW || by >= BH) return true;
                if (by >= 0 && board[by][bx]) return true;
            }
    return false;
}

static void rotate_cw(uint32_t *m) {
    uint32_t n[4] = {0};
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (m[r] & (0x8000 >> c))
                n[c] |= (0x8000 >> (3 - r));
    memcpy(m, n, sizeof n);
}

static void spawn(void) {
    shp_id = next_id;
    memcpy(shape, SHAPES[shp_id], sizeof shape);
    next_id = rand() % 7;
    px = 3;
    py = 0;
    if (collide(py, px, shape)) over = true;
}

static int clear_lines(void) {
    int cleared = 0;
    for (int r = BH - 1; r >= 0; r--) {
        bool full = true;
        for (int c = 0; c < BW; c++)
            if (!board[r][c]) { full = false; break; }
        if (full) {
            cleared++;
            for (int rr = r; rr > 0; rr--)
                memcpy(board[rr], board[rr - 1], sizeof board[0]);
            memset(board[0], 0, sizeof board[0]);
            r++;  /* 该行被替换下移，需重新检查 */
        }
    }
    return cleared;
}

/* 锁定当前方块、消行、计分、生成新方块 */
static void lock(void) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (shape[r] & (0x8000 >> c)) {
                int by = py + r, bx = px + c;
                if (by >= 0 && by < BH && bx >= 0 && bx < BW)
                    board[by][bx] = 1;
            }
    int n = clear_lines();
    if (n > 0) {
        static const int pts[5] = {0, 100, 300, 500, 800};
        lines += n;
        score += pts[n] * level;
        level = lines / 10 + 1;
        drop_ms = 650 - (level - 1) * 55;
        if (drop_ms < 45) drop_ms = 45;
    }
    spawn();
}

static void try_move(int dx) {
    if (!collide(py, px + dx, shape)) px += dx;
}

static void try_rotate(void) {
    uint32_t m[4];
    memcpy(m, shape, sizeof m);
    rotate_cw(m);
    int kicks[] = {0, -1, 1, -2, 2};
    for (unsigned i = 0; i < sizeof kicks / sizeof kicks[0]; i++) {
        if (!collide(py, px + kicks[i], m)) {
            memcpy(shape, m, sizeof m);
            px += kicks[i];
            return;
        }
    }
}

static bool step_down(void) {
    if (collide(py + 1, px, shape)) return false;
    py++;
    return true;
}

static void hard_drop(void) {
    while (step_down()) ;
    lock();
}

static int ghost_y(void) {
    int y = py;
    while (!collide(y + 1, px, shape)) y++;
    return y;
}

/* 绘制一格：style 决定颜色，s 是单字符块，横向重复两次（格子 2 字符宽） */
static void cell(int y, int x, int style, const char *s) {
    tui_set(style);
    tui_text(y, x, s);
    tui_text(y, x + 1, s);
}

static void render(int x0, int y0, bool paused) {
    int g = ghost_y();
    /* 主框 */
    tui_set(TUI_GREEN);
    tui_box(y0, x0, y0 + BH + 1, x0 + BW * 2 + 1);

    /* 先清空棋盘内容区，避免方块移动/旋转后旧位置残留 */
    tui_set(TUI_RESET);
    for (int r = 0; r < BH; r++)
        tui_fill(y0 + 1 + r, x0 + 1, BW * 2, ' ');

    /* 幽灵投影 */
    if (g > py)
        for (int r = 0; r < 4; r++)
            for (int c = 0; c < 4; c++)
                if (shape[r] & (0x8000 >> c)) {
                    int by = g + r, bx = px + c;
                    if (by >= 0 && by < BH && bx >= 0 && bx < BW && !board[by][bx])
                        cell(y0 + 1 + by, x0 + 1 + bx * 2, TUI_GREEN_DIM, "▓");
                }

    /* 已锁定棋盘 */
    for (int r = 0; r < BH; r++)
        for (int c = 0; c < BW; c++)
            if (board[r][c])
                cell(y0 + 1 + r, x0 + 1 + c * 2, TUI_GREEN, "█");

    /* 当前方块 */
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (shape[r] & (0x8000 >> c)) {
                int by = py + r, bx = px + c;
                if (by >= 0 && by < BH && bx >= 0 && bx < BW)
                    cell(y0 + 1 + by, x0 + 1 + bx * 2, TUI_GREEN_BRIGHT, "█");
            }

    /* 右侧信息面板（矮终端时面板底边贴住屏幕底，不溢出） */
    int ix = x0 + BW * 2 + 4;
    int pbot = y0 + 16;
    if (pbot >= tui_term_rows()) pbot = tui_term_rows() - 1;
    tui_set(TUI_GREEN);
    tui_box(y0, ix, pbot, ix + 16);
    tui_set(TUI_GREEN_DIM);
    tui_text(y0 + 1, ix + 2, "NEXT");
    /* 清空预览区，避免换下一个形状时旧方块残留 */
    tui_set(TUI_RESET);
    for (int r = 0; r < 4; r++)
        tui_fill(y0 + 2 + r, ix + 3, 14, ' ');
    tui_set(TUI_GREEN_BRIGHT);
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (SHAPES[next_id][r] & (0x8000 >> c))
                cell(y0 + 2 + r, ix + 3 + c * 2, TUI_GREEN_BRIGHT, "█");

    char buf[48];
    tui_set(TUI_YELLOW);
    tui_text(y0 + 7, ix + 2, "SCORE");
    snprintf(buf, sizeof(buf), "%06d", score);
    tui_text(y0 + 8, ix + 2, buf);
    tui_set(TUI_GREEN);
    tui_text(y0 + 10, ix + 2, "LEVEL");
    snprintf(buf, sizeof(buf), "%02d", level);
    tui_text(y0 + 11, ix + 2, buf);
    tui_text(y0 + 13, ix + 2, "LINES");
    snprintf(buf, sizeof(buf), "%03d", lines);
    tui_text(y0 + 14, ix + 2, buf);
    tui_set(TUI_RESET);

    /* 底部提示（单行，节省纵向空间；终端太矮时省略，避免溢出） */
    if (y0 + BH + 3 < tui_term_rows()) {
        tui_set(TUI_GREEN_DIM);
        tui_put_safe(y0 + BH + 3, x0, "[←→/AD]移动 [↑/W]旋转 [↓/S]软降 [空格]硬降 [P]暂停 [Q]退出");
    }
    if (paused) {
        tui_set(TUI_YELLOW);
        tui_text(y0 + 1, x0 + BW + 1 - 6, "╔══════════╗");
        tui_text(y0 + 2, x0 + BW + 1 - 6, "║  PAUSED  ║");
        tui_text(y0 + 3, x0 + BW + 1 - 6, "╚══════════╝");
    }
    tui_set(TUI_RESET);
    fflush(stdout);
}

static void show_over(int x0, int y0) {
    tui_set(TUI_RED);
    tui_text(y0 + BH / 2 - 1, x0 + BW + 1 - 9, "  GAME OVER  ");
    tui_set(TUI_YELLOW);
    char buf[48];
    snprintf(buf, sizeof(buf), "SCORE %06d  LINES %d", score, lines);
    tui_text(y0 + BH / 2 + 1, x0 + BW + 1 - 10, buf);
    tui_set(TUI_GREEN_DIM);
    tui_text(y0 + BH / 2 + 3, x0 + BW + 1 - 14, "[任意键] 返回大厅   [Q] 直接退出");
    tui_set(TUI_RESET);
    fflush(stdout);
}

int game_tetris_run(void) {
    srand((unsigned)(time(NULL) ^ (long)tui_pid() << 16));
    memset(board, 0, sizeof board);
    score = lines = 0;
    level = 1;
    drop_ms = 650;
    over = false;
    next_id = rand() % 7;
    spawn();

    int x0 = tui_center_x(BW * 2 + 24);
    int y0 = tui_center_row(BH + 6);
    int acc = 0;
    bool paused = false;

    while (!over) {
        Key k;
        while ((k = tui_getch_timeout(0)) != KEY_NONE) {
            switch (k) {
                case KEY_LEFT:  case KEY_A: try_move(-1); break;
                case KEY_RIGHT: case KEY_D: try_move(1);  break;
                case KEY_UP:    case KEY_W: try_rotate(); break;
                case KEY_DOWN:  case KEY_S:
                    if (!step_down()) lock();
                    else score += 1;  /* 软降微加分 */
                    break;
                case KEY_SPACE: hard_drop(); break;
                case KEY_P: paused = !paused; break;
                case KEY_Q: case KEY_ESC: return 0;
                default: break;
            }
        }
        if (paused) {
            render(x0, y0, true);
            tui_sleep(60);
            continue;
        }
        acc += 40;
        if (acc >= drop_ms) {
            acc = 0;
            if (!step_down()) lock();
        }
        render(x0, y0, false);
        tui_sleep(40);
    }

    render(x0, y0, false);
    show_over(x0, y0);
    for (;;) {
        Key k = tui_getch_timeout(1000);
        if (k != KEY_NONE) return 0;
    }
}
