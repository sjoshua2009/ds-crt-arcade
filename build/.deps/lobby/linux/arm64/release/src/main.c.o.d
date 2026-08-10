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
        "src/main.c"
    },
    depfiles = "build/.objs/lobby/linux/arm64/release/src/__cpp_main.c.c: src/main.c   src/games.h src/tui.h\
"
}