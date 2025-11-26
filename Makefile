# Nome do programa (aparece em PRGM)
NAME := EX1LAMBD
DESCRIPTION := "EX1"
COMPRESSED := YES
ARCHIVED := YES

# Só o viewer de AppVar:
SRC := src/main.c

# Flags e libs
CFLAGS  := -Wall -Wextra -Oz
LIBS    := -lgraphx -lkeypadc -lfileioc -ltice -lm -graphx -fontlibc -fileioc -keypadc

include $(shell cedev-config --makefile)
