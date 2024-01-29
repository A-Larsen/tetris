LIBS = -lSDL2 -lm
build: tetris.c
	gcc -o tetris tetris.c $(LIBS)
