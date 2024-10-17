CC = g++
FLAGS = -Wall -Wextra -Werror -lncurses

tetris:
	$(CC) tetris.cpp $(FLAGS) -o tetris
	./tetris

clean:
	rm tetris

.PHONY: tetris