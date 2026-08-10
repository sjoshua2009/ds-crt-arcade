/* 2048 算法单元测试：直接 include 真实实现，验证滑动/合并/方向映射 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "g2048.c"

static int fails = 0;

static void check_row(const char *name, int *got, int g0, int *exp, int e0) {
    for (int i = 0; i < 4; i++)
        if (got[i] != exp[i] || g0 != e0) {
            printf("FAIL %s: got [%d %d %d %d] g=%d want [%d %d %d %d] g=%d\n",
                   name, got[0], got[1], got[2], got[3], g0, exp[0], exp[1], exp[2], exp[3], e0);
            fails++;
            return;
        }
    printf("PASS %s\n", name);
}

static void set_board(const int m[4][4]) { memcpy(b, m, sizeof b); }

static void check_board(const char *name, const int exp[4][4]) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (b[r][c] != exp[r][c]) {
                printf("FAIL %s: got %d want %d\n", name, b[r][c], exp[r][c]);
                fails++;
                return;
            }
    printf("PASS %s\n", name);
}

int main(void) {
    no_random = true;   /* 移动不生成随机块，便于断言 */

    /* slide_row */
    int r1[4] = {2, 2, 2, 2}, e1[4] = {4, 4, 0, 0};
    int g = slide_row(r1); check_row("slide 2222", r1, g, e1, 8);
    int r2[4] = {2, 2, 4, 0}, e2[4] = {4, 4, 0, 0};
    g = slide_row(r2); check_row("slide 2240", r2, g, e2, 4);
    int r3[4] = {4, 4, 8, 8}, e3[4] = {8, 16, 0, 0};
    g = slide_row(r3); check_row("slide 4488", r3, g, e3, 24);
    int r4[4] = {2, 2, 2, 4}, e4[4] = {4, 2, 4, 0};
    g = slide_row(r4); check_row("slide 2224(仅合并一次)", r4, g, e4, 4);
    int r5[4] = {2, 4, 2, 4}, e5[4] = {2, 4, 2, 4};
    g = slide_row(r5); check_row("slide 2424(无可合并)", r5, g, e5, 0);

    /* move_dir 方向映射 */
    static const int up_in[4][4] = {{2,0,0,0},{0,0,0,0},{2,0,0,0},{0,0,0,0}};
    static const int up_out[4][4] = {{4,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    set_board(up_in); move_dir(2); check_board("上移合并", up_out);

    static const int dn_in[4][4] = {{2,0,0,0},{0,0,0,0},{2,0,0,0},{0,0,0,0}};
    static const int dn_out[4][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{4,0,0,0}};
    set_board(dn_in); move_dir(3); check_board("下移合并", dn_out);

    static const int rt_in[4][4] = {{0,0,2,2},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    static const int rt_out[4][4] = {{0,0,0,4},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    set_board(rt_in); move_dir(1); check_board("右移合并", rt_out);

    static const int lf_in[4][4] = {{2,2,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    static const int lf_out[4][4] = {{4,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    set_board(lf_in); move_dir(0); check_board("左移合并", lf_out);

    /* 不可移动时不生成新块 */
    static const int full[4][4] = {{2,4,2,4},{4,2,4,2},{2,4,2,4},{4,2,4,2}};
    set_board(full);
    int before[4][4]; memcpy(before, b, sizeof b);
    bool moved = move_dir(0);
    if (moved || memcmp(before, b, sizeof b) != 0) {
        printf("FAIL 无可移动时不应变化\n"); fails++;
    } else printf("PASS 无变化移动\n");

    /* can_move */
    set_board(full);
    if (can_move()) { printf("FAIL 全异格应判死局\n"); fails++; }
    else printf("PASS 死局检测\n");
    static const int movable[4][4] = {{2,2,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    set_board(movable);
    if (!can_move()) { printf("FAIL 应有路可走\n"); fails++; }
    else printf("PASS 活局检测\n");

    /* 长链 2048 获胜检测 */
    int r2048[4] = {1024, 1024, 0, 0};
    won = false;
    slide_row(r2048);
    if (!won) { printf("FAIL 合并出 2048 应判胜\n"); fails++; }
    else printf("PASS 2048 获胜检测\n");

    printf(fails ? "\n== %d 项失败 ==\n" : "\n== 全部通过 ==\n", fails);
    return fails ? 1 : 0;
}
