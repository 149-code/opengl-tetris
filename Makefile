all:
	g++-10 tetris.cpp -std=c++20 -framework OpenGl -lglfw -lglew -I/usr/local/include -L/usr/local/lib
