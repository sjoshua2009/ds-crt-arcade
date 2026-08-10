# ds-crt-arcade

> A game lobby running in tui · 复古 CRT 绿风格终端游戏大厅

纯 ANSI 转义序列 + termios raw mode 实现的 TUI 游戏大厅，内置 6 款经典小游戏。C11 编写，xmake 构建，无任何第三方依赖，在 Windows / macOS / Linux / Termux 任意终端中都能运行。

## 特性

- 复古 CRT 绿色单色主题：暗绿 / 亮绿 / 黄 / 反白配色，双线框装饰，块字标题横幅
- 6 款内置游戏：贪吃蛇、俄罗斯方块、2048、井字棋、扫雷、乒乓
- 完整大厅场景：高亮菜单、`1-6` 快捷进入、进出游戏全屏切换、`Q` 退出
- 小终端自适应：画布、棋盘、拍子随终端自动收缩，长文本防折行，矮终端自动省略提示行
- 每帧全量重绘，无画面残影
- 方向键 + WASD 双操控，各游戏内置暂停 / 重开 / 退出
- 跨平台 CI：一键产出 Windows / macOS / Linux 的 x64 与 arm64 二进制

## 技术栈

- C11 · [xmake](https://xmake.io) 构建 · clang 编译器
- 无第三方依赖：全部终端控制走 ANSI 转义序列 + termios

## 快速开始

```sh
xmake                                   # 构建
./build/linux/arm64/release/lobby       # 运行（具体路径以 xmake 输出为准）
```

运行测试（2048 与俄罗斯方块逻辑，共 26 个断言）：

```sh
clang -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections -I src \
  test/test_t2048.c src/tui.c -o .t2048 && ./.t2048
clang -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections -I src \
  test/test_tetris.c src/tui.c -o .ttetris && ./.ttetris
```

## 操作

| 场景 | 按键 |
|------|------|
| 大厅 | `↑↓` / `WS` 选择，`Enter` 进入，`1-6` 快捷进入，`Q` 退出 |
| 游戏内 | 方向键 / `WASD` 操控，`P` 暂停，`R` 重开，`Q` 返回大厅 |

## 游戏

| 键 | 游戏 | 说明 |
|----|------|------|
| 1 | 贪吃蛇 | 吃掉食物不断变长，速度随之加快 |
| 2 | 俄罗斯方块 | 消行挑战高分，幽灵投影辅助落点 |
| 3 | 2048 | 方向键滑动合成，挑战 2048 |
| 4 | 井字棋 | 和电脑下三子棋 |
| 5 | 扫雷 | 推理排除所有地雷，首次点击必安全 |
| 6 | 乒乓 | 和电脑对战到 7 分 |

## 跨平台二进制

通过 GitHub Actions 自动构建，产物上传到 Actions 页面可下载：

| 平台 | 架构 | 产物 |
|------|------|------|
| Windows | x64 / arm64* | `ds-crt-arcade-windows-*.zip` |
| macOS | x64 / arm64 | `ds-crt-arcade-macos-*.zip` |
| Linux | x64 / arm64 | `ds-crt-arcade-linux-*.zip` |

\* Windows arm64 需要交叉编译工具链，CI 中标记为实验性构建。

获取方式：
1. 仓库 **Actions** 页面 → 选择一个 workflow run → **Artifacts** 下载；
2. 或 `workflow_dispatch` 手动触发一次构建；
3. 打 `v*` 标签会一并触发构建，配合 Release 发布。

## 小终端适配

大厅和游戏会根据终端尺寸自动收缩：大厅画布动态缩放，贪吃蛇高度、乒乓场尺寸运行时计算，俄罗斯方块信息面板贴底、提示行在矮终端省略。已验证 `44x16`、`50x20`、`80x24` 均正常排版；俄罗斯方块的 10x20 棋盘是物理下限，需要 22 行终端。

## 目录结构

```
src/           游戏与大厅源码（tui 终端封装 + 6 款游戏 + main 大厅）
test/          2048 / 俄罗斯方块逻辑单元测试
check_size.sh  小终端布局冒烟验证脚本（sh + node）
.github/workflows/build.yml   跨平台构建 CI
```
