#ifndef TUI_H
#define TUI_H

#include <stdbool.h>

/* 统一按键码：屏蔽终端差异（方向键转义序列、大小写等） */
typedef enum {
    KEY_NONE = 0,
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_ENTER, KEY_ESC, KEY_SPACE, KEY_TAB,
    KEY_Q, KEY_P, KEY_F, KEY_R,
    KEY_W, KEY_A, KEY_S, KEY_D,
    KEY_1, KEY_2, KEY_3, KEY_4, KEY_5,
    KEY_6, KEY_7, KEY_8, KEY_9, KEY_0,
    KEY_OTHER
} Key;

/* CRT 绿主题样式 */
enum {
    TUI_GREEN_DIM = 0,   /* 暗绿：装饰、未选中 */
    TUI_GREEN,           /* 常规绿 */
    TUI_GREEN_BRIGHT,    /* 亮绿：高亮、标题 */
    TUI_YELLOW,          /* 黄：警告、分数、食物 */
    TUI_RED,             /* 红：死亡、炸弹 */
    TUI_INVERSE,         /* 反白黑底亮绿：选中项、光标 */
    TUI_RESET
};

void  tui_init(void);                 /* 进入 raw 模式、隐藏光标 */
void  tui_restore(void);              /* 恢复终端 */
void  tui_clear(void);                /* 清屏并回到左上角 */
void  tui_move(int row, int col);     /* 光标定位，1-based */
void  tui_set(int style);
void  tui_box(int r0, int c0, int r1, int c1);          /* 双线框 */
void  tui_hline(int r, int c0, int c1, int style);      /* 水平实线 */
void  tui_text(int r, int c, const char *s);            /* 定位输出字符串 */
void  tui_printf(int r, int c, const char *fmt, ...);   /* 定位格式化输出 */
void  tui_fill(int r, int c, int count, char ch);       /* 从 (r,c) 起填充 count 个单字符 */
void  tui_put_safe(int r, int c, const char *s);        /* 输出但不超出右边界（UTF-8 不截半） */
int   tui_term_cols(void);
int   tui_term_rows(void);
int   tui_center_x(int width);        /* 计算使 width 列内容居中的起始列 */
int   tui_center_row(int height);
Key   tui_getch_timeout(int ms);      /* 最多等待 ms 毫秒，超时返回 KEY_NONE */
Key   tui_wait_any(void);             /* 阻塞等待任意按键 */
void  tui_sleep(int ms);              /* 跨平台休眠（游戏帧率控制） */
long  tui_pid(void);                  /* 跨平台进程号（rand 播种用） */

#endif
