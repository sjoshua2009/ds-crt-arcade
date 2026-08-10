{
    depfiles_format = "gcc",
    values = {
        "clang",
        {
            "-Qunused-arguments",
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-O2"
        }
    },
    files = {
        "src/minesweeper.c"
    },
    depfiles = "build/.objs/lobby/linux/arm64/release/src/__cpp_minesweeper.c.c:   src/minesweeper.c src/games.h src/tui.h\
"
}