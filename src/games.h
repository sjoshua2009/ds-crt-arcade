#ifndef GAMES_H
#define GAMES_H

/* 每个游戏的入口函数：进入全屏运行，返回后回到大厅。
 * 约定返回 0 = 正常返回大厅。 */
typedef int (*game_fn)(void);

typedef struct {
    const char *id;    /* 短编号，如 "01" */
    const char *name;  /* 英文名，如 "SNAKE" */
    const char *zh;    /* 中文名 */
    const char *tag;   /* 类型标签，如 [经典] */
    const char *desc;  /* 一句简介 */
    game_fn fn;
} game_t;

extern game_t g_games[];
extern int     g_game_count;

int game_snake_run(void);
int game_tetris_run(void);
int game_2048_run(void);
int game_tictactoe_run(void);
int game_minesweeper_run(void);
int game_pong_run(void);

#endif
