LIBS = -lSDL2 -lSDL2_ttf -lm
build: tetris.c
	gcc -g -o tetris tetris.c $(LIBS)
