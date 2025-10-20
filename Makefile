# Nome do programa (aparece em PRGM)
NAME = LaTeX
DESCRIPTION = "LaTeX (demo CE C)"
COMPRESSED = YES
ARCHIVED = YES

# Fontes
SRC = src/tex2ce.c latex_viewer_ce.c

# Otimizacoes e libs (graphx, keypadc, tice)
CFLAGS = -Wall -Wextra -Oz
LDFLAGS = -lgraphx -lkeypadc -ltice -lm


include $(shell cedev-config --makefile)