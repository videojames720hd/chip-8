EXEC = CHIP8
CC = g++
OPT = -O2

CFLAGS = -std=c++17 -Wall ${OPT} -lSDL3

SRC = $(wildcard *.cpp)
HDR = $(wildcard *.hpp)

${EXEC}: ${SRC} ${HDR}
	${CC} ${CFLAGS} ${SRC} -o ${EXEC}

.PHONY: clean

clean:
	rm ${EXEC}
