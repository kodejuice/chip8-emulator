
FLAG = -O3 -fomit-frame-pointer -g -std=c++14# -Wall -Wno-unused-function -Wno-unused-result -Wall -Wextra -Wshadow


build:
	clang++ main.cc $(FLAG) -o chip8


