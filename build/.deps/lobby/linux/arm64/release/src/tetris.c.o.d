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
        "src/tetris.c"
    },
    depfiles = "build/.objs/lobby/linux/arm64/release/src/__cpp_tetris.c.c: src/tetris.c   src/games.h src/tui.h\
"
}