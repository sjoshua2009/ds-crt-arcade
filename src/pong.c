#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "games.h"
#include "tui.h"

#define PW_MAX 48
#define PH_MAX 14
#define PAD_H 5
#define WIN_SCORE 7

static int pw, ph;              /* 运行时尺寸：按终端自动缩小 */
static int p_top;               /* 玩家拍子顶部 y */
static int c_top;               /* 电脑拍子顶部 y */
static float bx, by, vx, vy;    /* 球位置与速度 */
static int pscore, cscore;
static bool over, paused;

static const int PAD_X = 1;     /* 玩家拍子列 */
static int cpad_x;              /* 电脑拍子列 = pw - 2 */

static void reset_ball(int dir) {
    bx = pw / 2.0f;
    by = ph / 2.0f;
    vx = dir * 5.0f;
    vy = ((float)(rand() % 5) - 2.0f) * 0.8f;
    if (vy == 0) vy = 0.8f;
}

static void reset(void) {
    p_top = ph / 2 - PAD_H / 2;
    c_top = ph / 2 - PAD_H / 2;
    pscore = cscore = 0;
    over = paused = false;
    reset_ball((rand() % 2) ? 1 : -1);
}

static void step(void) {
    /* 玩家输入已在外层处理；这里做物理推进 */
    bx += vx;
    by += vy;

    /* 上下墙反弹 */
    if (by < 0.5f) { by = 0.5f; vy = -vy; }
    if (by > ph - 1.5f) { by = ph - 1.5f; vy = -vy; }

    /* 玩家拍子碰撞 */
    if (vx < 0 && bx <= PAD_X + 0.5f && by >= p_top - 1 && by <= p_top + PAD_H) {
        float rel = (by - (p_top + PAD_H / 2.0f)) / (PAD_H / 2.0f);
        if (rel > 1.0f) rel = 1.0f;
        if (rel < -1.0f) rel = -1.0f;
        vx = 5.0f + (float)pscore * 0.15f;
        vy = rel * 4.5f;
        bx = PAD_X + 0.6f;
    }
    /* 电脑拍子碰撞 */
    if (vx > 0 && bx >= cpad_x - 0.5f && by >= c_top - 1 && by <= c_top + PAD_H) {
        float rel = (by - (c_top + PAD_H / 2.0f)) / (PAD_H / 2.0f);
        if (rel > 1.0f) rel = 1.0f;
        if (rel < -1.0f) rel = -1.0f;
        vx = -(5.0f + (float)cscore * 0.15f);
        vy = rel * 4.5f;
        bx = cpad_x - 0.6f;
    }

    /* 得分 */
    if (bx < -0.5f) { cscore++; reset_ball(-1); }
    else if (bx > pw + 0.5f) { pscore++; reset_ball(1); }
    if (pscore >= WIN_SCORE || cscore >= WIN_SCORE) over = true;
}

static void render(int x0, int y0) {
    char line[PW_MAX + 1];
    tui_set(TUI_GREEN);
    tui_box(y0, x0, y0 + ph + 1, x0 + pw + 1);

    for (int r = 0; r < ph; r++) {
        memset(line, ' ', pw);
        line[pw] = '\0';
        /* 中线 */
        if (r % 2 == 0) line[pw / 2] = '.';
        /* 拍子 */
        if (r >= p_top && r < p_top + PAD_H) line[PAD_X] = '#';
        if (r >= c_top && r < c_top + PAD_H) line[cpad_x] = '#';
        /* 球 */
        if ((int)by == r) {
            int bc = (int)bx;
            if (bc >= 0 && bc < pw) line[bc] = 'o';
        }
        tui_text(y0 + 1 + r, x0 + 1, line);
    }

    /* 计分 */
    char buf[48];
    tui_set(TUI_YELLOW);
    snprintf(buf, sizeof buf, "YOU %02d", pscore);
    tui_text(y0, x0 + 4, buf);
    tui_set(TUI_GREEN_BRIGHT);
    snprintf(buf, sizeof buf, " : %02d", cscore);
    tui_text(y0, x0 + 13, buf);

    tui_set(TUI_GREEN_DIM);
    tui_put_safe(y0 + ph + 2, x0, "[W/S] 上下移动  [P] 暂停  [R] 重开  [Q] 退出");
    tui_set(TUI_RESET);

    if (paused) {
        tui_set(TUI_YELLOW);
        tui_text(y0 + ph / 2, x0 + pw / 2 - 6, "╔════════════╗");
        tui_text(y0 + ph / 2 + 1, x0 + pw / 2 - 6, "║   PAUSED   ║");
        tui_text(y0 + ph / 2 + 2, x0 + pw / 2 - 6, "╚════════════╝");
        tui_set(TUI_RESET);
    }
    if (over) {
        bool win = pscore > cscore;
        tui_set(win ? TUI_GREEN_BRIGHT : TUI_RED);
        tui_text(y0 + ph / 2, x0 + pw / 2 - 5, win ? "  ★ YOU WIN ★  " : "  YOU LOSE ...  ");
        tui_set(TUI_GREEN_DIM);
        tui_text(y0 + ph / 2 + 2, x0 + pw / 2 - 10, "[任意键] 返回大厅  [R] 再来一局");
        tui_set(TUI_RESET);
    }
    fflush(stdout);
}

int game_pong_run(void) {
    srand((unsigned)(time(NULL) ^ (long)tui_pid() << 3));
    /* 尺寸随终端自动收缩（上限 PW_MAX/PH_MAX，留出最小可玩尺寸） */
    pw = tui_term_cols() - 4;
    ph = tui_term_rows() - 4;
    if (pw > PW_MAX) pw = PW_MAX;
    if (ph > PH_MAX) ph = PH_MAX;
    if (pw < 22) pw = 22;
    if (ph < 6) ph = 6;
    cpad_x = pw - 2;
    reset();
    int x0 = tui_center_x(pw + 2);
    int y0 = tui_center_row(ph + 5);

    for (;;) {
        Key k;
        while ((k = tui_getch_timeout(0)) != KEY_NONE) {
            switch (k) {
                case KEY_W: if (p_top > 0) p_top--; break;
                case KEY_S: if (p_top < ph - PAD_H) p_top++; break;
                case KEY_UP: if (p_top > 0) p_top--; break;
                case KEY_DOWN: if (p_top < ph - PAD_H) p_top++; break;
                case KEY_P: paused = !paused; break;
                case KEY_R: reset(); break;
                case KEY_Q: case KEY_ESC: return 0;
                default: break;
            }
        }
        if (over) {
            render(x0, y0);
            Key w = tui_getch_timeout(2000);
            if (w == KEY_R) { reset(); continue; }
            return 0;
        }
        if (!paused) {
            /* 电脑 AI：追球，留一点失误空间 */
            int target = (int)by;
            int ccenter = c_top + PAD_H / 2;
            int speed = 2;
            if (vx > 0) speed = 3;
            if (ccenter < target - 2 && c_top < ph - PAD_H) c_top += speed;
            else if (ccenter > target + 2 && c_top > 0) c_top -= speed;
            step();
        }
        render(x0, y0);
        tui_sleep(16);
    }
}
