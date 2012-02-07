tools/test:	tools/test.cc src/film.cc
		g++ -Wall -g -D__STDC_CONSTANT_MACROS -o $@ tools/test.cc src/film.cc -Isrc `pkg-config --cflags --libs libavformat libswscale` -ltiff

clean:;		rm -f tools/test
