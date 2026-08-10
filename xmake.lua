set_languages("c11")
add_cflags("-Wall -Wextra -O2", {force = true})
set_toolchains("clang")

target("lobby")
    set_kind("binary")
    add_files("src/*.c")
