cg2:
	gcc src/cg2.c -O3 -Wall -lSDL2 -o build/cg2

all: cg2 

clean:
	rm build/cg2
