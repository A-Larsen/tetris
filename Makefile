LIBS = -lSDL2 -lm
build: tetris.c
	gcc -g -o tetris tetris.c $(LIBS)
