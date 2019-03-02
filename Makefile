# FLAG = -Wall -Wno-unused-function -Wno-unused-result -Wall -Wextra -Wshadow

OBJS = main.cc

CC = clang++

COMPILER_FLAGS = -w -O3 -fomit-frame-pointer -g -std=c++14

LINKER_FLAGS = -lSDL2

OBJ_NAME = chip8

all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
