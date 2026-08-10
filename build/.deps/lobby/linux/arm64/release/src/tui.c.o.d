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
        "src/tui.c"
    },
    depfiles = "build/.objs/lobby/linux/arm64/release/src/__cpp_tui.c.c: src/tui.c   src/tui.h\
"
}