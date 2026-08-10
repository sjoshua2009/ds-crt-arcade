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
        "src/pong.c"
    },
    depfiles = "build/.objs/lobby/linux/arm64/release/src/__cpp_pong.c.c: src/pong.c   src/games.h src/tui.h\
"
}