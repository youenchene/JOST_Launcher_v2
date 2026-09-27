cmake_minimum_required(VERSION 3.14.0)
project(jl LANGUAGES C)

if(NOT AMIGA)
	message(SEND_ERROR "Amiga only - build with: agk build (host tests: agk unit)")
endif()

set(CMAKE_C_STANDARD 11)
# OS-friendly Kickstart 1.3 program: libnix for 1.3 (M68K_CRT=nix13, set by
# the kit's tools/build), no ACE, no C library I/O. -Os: it's a menu, size
# matters more than speed (it only redraws what changed).
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -Wall -Wextra -fomit-frame-pointer -fno-common")

file(GLOB SOURCES src/*.c)  # not src/stub
add_executable(jl ${SOURCES})

# Test-disk helper (tools/mkdisk.py puts it in C: of the test disk only)
add_executable(palfix tests/tools/palfix.c)
# Benchmark helper (tools/bench.py installs it as C:jst on the bench disks)
add_executable(memreport tests/tools/memreport.c src/dbg.c)
# SPIKE C: resident stub (C:jlstub) + jl as the menu module
add_executable(jlstub src/stub/stub.c src/launch.c src/sys.c src/dbg.c)
