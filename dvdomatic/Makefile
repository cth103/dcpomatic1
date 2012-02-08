tools/test:	tools/test.cc src/film.cc src/format.cc src/film_writer.cc
		g++ -Wall -g -D__STDC_CONSTANT_MACROS -o $@ tools/test.cc src/film.cc src/format.cc src/film_writer.cc -Isrc `pkg-config --cflags --libs libavformat libavfilter libswscale libswresample` -ltiff -lsndfile

clean:;		rm -f tools/test
