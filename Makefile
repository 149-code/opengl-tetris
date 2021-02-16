dynamic:
	c++ tetris.cpp -std=c++17 -framework OpenGl -lglfw -lglew -I/usr/local/include -L/usr/local/lib

static:
	c++ tetris_static.cpp -std=c++17 libglfw3.a libGLEW.a -framework OpenGl -framework CoreFoundation -framework CoreGraphics -framework AppKit -framework IOKit

clean:
	rm a.out
