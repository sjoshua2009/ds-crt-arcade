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
        "src/snake.c"
    },
    depfiles = "build/.objs/lobby/linux/arm64/release/src/__cpp_snake.c.c: src/snake.c   src/games.h src/tui.h\
"
}