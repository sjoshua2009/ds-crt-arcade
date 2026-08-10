set_languages("c11")
add_cflags("-Wall -Wextra -O2", {force = true})
set_toolchains("clang")

target("lobby")
    set_kind("binary")
    add_files("src/*.c")

    -- Windows 终端控制走 PDCurses（CI 构建前把源码拉取到 deps/pdcurses）；
    -- POSIX（Linux/macOS/Termux）保持纯 ANSI + termios，无需额外依赖。
    if is_plat("windows") then
        add_includedirs("deps/pdcurses")
        add_files(
            "deps/pdcurses/pdcurses/*.c",
            "deps/pdcurses/wincon/*.c")
        add_defines("PDC_WIDE")   -- 启用宽字符接口（mvaddwstr 等）
        add_syslinks("user32", "advapi32")
    end
