# Force format
__A := $(shell astyle --style=allman cube.c)

TOP_DIR=../..
MODULE_NAME=mltetris
MODULE_OBJS=tetris.o
include $(TOP_DIR)/modules/Makefile.modules
