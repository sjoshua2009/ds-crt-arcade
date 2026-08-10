/* 俄罗斯方块逻辑单元测试：include 真实实现 */
#include <stdio.h>
#include <string.h>
#include "tetris.c"

static int fails = 0;

#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL %s\n", msg); fails++; } \
    else printf("PASS %s\n", msg); } while (0)

int main(void) {
    /* 旋转：I 横变竖 */
    uint32_t i[4] = {0xF000, 0, 0, 0};
    rotate_cw(i);
    CHECK(i[0] == 0x1000 && i[1] == 0x1000 && i[2] == 0x1000 && i[3] == 0x1000, "I 旋转竖直");

    /* 旋转 4 次回到原形 */
    uint32_t t[4] = {0x4000, 0xE000, 0, 0};
    uint32_t t0[4]; memcpy(t0, t, sizeof t0);
    for (int k = 0; k < 4; k++) rotate_cw(t);
    CHECK(memcmp(t, t0, sizeof t) == 0, "T 旋转 4 次复原");

    /* 碰撞检测：I 位于 (3,1) 会出右边界 */
    memset(board, 0, sizeof board);
    shp_id = 0;
    memcpy(shape, SHAPES[0], sizeof shape);
    CHECK(!collide(0, 0, shape), "I 顶部不碰撞");
    CHECK(collide(0, 7, shape), "I 出右界碰撞");
    CHECK(collide(20, 0, shape), "I 出下界碰撞");

    /* 碰撞：T 形 */
    uint32_t tt[4] = {0x4000, 0xE000, 0, 0};
    CHECK(collide(19, 0, tt), "T 贴底碰撞");

    /* 消行 */
    for (int c = 0; c < BW; c++) board[19][c] = 1;   /* 一行满 */
    board[18][0] = 1;                                 /* 上一行留一块 */
    int n = clear_lines();
    CHECK(n == 1, "消除一行");
    CHECK(board[19][0] == 1, "消行后上移");
    CHECK(board[19][1] == 0, "消行后补空");
    CHECK(board[18][0] == 0, "被消行清空");

    /* 四行全消 */
    memset(board, 0, sizeof board);
    for (int r = 16; r <= 19; r++)
        for (int c = 0; c < BW; c++) board[r][c] = 1;
    n = clear_lines();
    CHECK(n == 4, "四行同消");

    /* 幽灵投影 */
    memset(board, 0, sizeof board);
    memcpy(shape, SHAPES[1], sizeof shape);   /* O 形 */
    px = 4; py = 0;
    CHECK(ghost_y() == BH - 2, "幽灵投影到底");
    board[18][4] = 1; board[18][5] = 1;        /* 阻挡 */
    CHECK(ghost_y() == 16, "幽灵投影遇障停");

    printf(fails ? "\n== %d 项失败 ==\n" : "\n== 全部通过 ==\n", fails);
    return fails ? 1 : 0;
}
