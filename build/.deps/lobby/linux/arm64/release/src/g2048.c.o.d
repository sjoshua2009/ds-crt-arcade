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
        "src/g2048.c"
    },
    depfiles = "build/.objs/lobby/linux/arm64/release/src/__cpp_g2048.c.c: src/g2048.c   src/games.h src/tui.h\
"
}