LIBS = -lSDL2 -lSDL2_ttf -lm

PROG = tetris

build: $(PROG).c
	gcc -g -o $(PROG) $(PROG).c $(LIBS)

clean:
	rm -rf $(PROG)

.PHONY: clean
