default:
	g++ main.cpp -lncurses -Wall -std=c++11 -o game

run: default
	./game
clean:
	-rm -f game

