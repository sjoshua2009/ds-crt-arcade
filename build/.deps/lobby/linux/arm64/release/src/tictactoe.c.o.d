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
        "src/tictactoe.c"
    },
    depfiles = "build/.objs/lobby/linux/arm64/release/src/__cpp_tictactoe.c.c:   src/tictactoe.c src/games.h src/tui.h\
"
}