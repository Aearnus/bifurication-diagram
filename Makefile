all:
	g++ -g -m64 -std=c++1y main.cpp -lSDL2 -lSDL2_ttf -pthread
