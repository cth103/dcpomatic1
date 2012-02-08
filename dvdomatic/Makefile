tools/test:	tools/test.cc src/film.cc src/format.cc src/film_writer.cc src/progress.cc
		g++ -Wall -g -D__STDC_CONSTANT_MACROS -o $@ tools/test.cc src/film.cc src/format.cc src/film_writer.cc src/progress.cc -Isrc `pkg-config --cflags --libs libavformat libavfilter libswscale libswresample glib-2.0` -ltiff -lsndfile -lboost_thread

clean:;		rm -f tools/test
