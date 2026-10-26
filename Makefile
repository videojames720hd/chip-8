EXEC = CHIP8
CC = g++
OPT = -O2

CFLAGS = -std=c++17 -Wall ${OPT} -Iinclude -lSDL3

SRC = $(wildcard src/*.cpp)
HDR = $(wildcard include/*.hpp)

${EXEC}: ${SRC} ${HDR}
	${CC} ${CFLAGS} ${SRC} -o ${EXEC}

.PHONY: clean

clean:
	rm ${EXEC}
