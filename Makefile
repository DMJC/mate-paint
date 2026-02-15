build:
	g++ mate-paint.cpp -o mate-paint `pkg-config --cflags --libs gtk+-3.0` -std=c++11
clean:
	rm mate-paint
