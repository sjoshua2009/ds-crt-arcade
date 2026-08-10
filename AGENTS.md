# ds-crt-arcade — 项目规范与开发经验

复古 CRT 绿风格 TUI 游戏大厅。C11 · xmake · clang。POSIX 平台零依赖（纯 ANSI + termios），Windows 终端控制用 PDCurses（CI 构建时拉取源码到 `deps/pdcurses`）。

## 构建 / 测试

```sh
xmake                       # 构建；产物在 build/<平台>/<架构>/<mode>/lobby
xmake f -y -m release       # 配置（CI 里还会 -p <platform> -a <arch>）
```

- 单元测试：2048 与俄罗斯方块逻辑，共 26 个断言。命令见 README（`clang -O2 ... test/test_t2048.c src/tui.c`）。
- 布局冒烟：`bash check_size.sh <cols> <rows> [游戏键]`（sh + node 解析 ANSI 光标码判断是否溢出）。环境变量 `BIN` 可覆盖被测二进制。
- **本机是 Termux（bionic libc）**：本地编译通过≠ glibc 通过（见下文陷阱 1）。CI 才是 glibc/macOS 的真实验证。
- **本机 `/tmp` 不存在、`/` 只读**（Android，非 root）：临时文件一律用 `$TMPDIR`（`$PREFIX/tmp`，可写），不要写死 `/tmp` 路径。

## 编码规范（重要）

- **游戏与大厅代码禁止直接输出或调用 POSIX 系统函数**：不得出现 `printf`/`fputs`/`getpid`/`usleep`/`nanosleep`，不得 `#include <unistd.h>/<poll.h>/<termios.h>`。
- 一切终端输出与系统调用走 `tui.h` 抽象：`tui_text / tui_printf / tui_fill / tui_put_safe / tui_sleep / tui_pid / tui_move / tui_set / tui_box / tui_hline`。
- 平台分叉只允许出现在 `src/tui.c` 内部（`#ifdef _WIN32` 两套后端）。新增游戏时同样只依赖 `tui_*` 接口。
- 组串用 `snprintf` 到局部缓冲；多字节 UTF-8（╔ ═ 等）只能出现在字符串字面量里，**不能**写进 `char` 字面量（C 的 char 是单字节，clang 会直接报错）。

## 踩过的坑（务必避开）

1. **glibc 严格 C11 不声明 POSIX 符号**：`set_languages("c11")` → `-std=c11` → 定义 `__STRICT_ANSI__`，glibc 下 `usleep/nanosleep/getpid` 全部未声明（Termux 的 bionic 没这问题，所以本地编过了 CI 才炸）。解决：`src/tui.c` 顶部、任何系统头文件之前 `#define _DEFAULT_SOURCE 1`（仅非 _WIN32）。
2. **PDCurses 的 KEY_* 宏与 tui.h 枚举重名**（`KEY_UP/DOWN/LEFT/RIGHT/ENTER`）。解决：`#include <curses.h>` 之后、`#undef` 之前用 `enum { TUI_PDC_KEY_UP = KEY_UP, ... }` 先捕获 PDCurses 按键码。**不能用 `#define TUI_PDC_KEY_UP KEY_UP` 另存**——宏在使用点才展开，届时 KEY_* 已被 undef，会错误解析回 tui.h 枚举值。
3. **PDCurses 构建要点**：必须 `-DPDC_WIDE`（才声明 `mvaddwstr` 等宽字符接口）；链接 `user32 advapi32`；`add_includedirs("deps/pdcurses")` 即可（wincon 端口用相对路径引 `../common/acs*.h`，core 的 debug.c 引 `sys/types.h` 在 UCRT 里存在，能编过）。
4. `deps/` 是 CI 拉取的第三方源码，已 gitignore，**不入库**。Windows 本地构建要先手动拉 PDCurses 3.9（命令见 README）。
5. **xmake 3.0.x 无 `--show-output-dir`**（CI 曾因此失败，`Invalid flag`）。定位产物统一用 `find build -type f \( -name lobby -o -name lobby.exe \) -print -quit`。
6. **macos-x64 已从矩阵移除**：macos-13 runner 在 GitHub Actions 上经常排队数十分钟，会拖死 `needs: build` 的 release job，而 macOS 已由 arm64 覆盖，收益与等待不成比例，故去掉。

## CI / 发布流程

- `.github/workflows/build.yml`：
  - `build` 矩阵：windows×{x64, arm64*}、macos×arm64、linux×{x64, arm64}。**windows-arm64 标记 experimental**（`continue-on-error`）——x64 的 clang 无法交叉链接 arm64，预期失败、不阻塞其他产物。
  - Windows job：先 `choco install llvm`（避开 MSVC 对 C11 复合字面量的限制），再拉 PDCurses 3.9。
  - `release` job：仅 `v*` 标签触发，`needs: build`，`permissions: contents: write`。下载全部矩阵产物 → 重命名 `ds-crt-arcade-<平台>-<架构>`（Windows 加 `.exe`）→ `gh release create` 发布并附运行说明。
- **发布**：`git tag -a v1.0.0 -m "..." && git push origin v1.0.0` → 自动全平台构建 + 发布到 Releases。产物也可从 Actions 页面 Artifacts 下载。
- 注意 bash 细节：`set -euo pipefail` 下反引号要转义（`\``）避免命令替换；`case "$name" in *windows*) ext=".exe";; esac` 判断平台。
