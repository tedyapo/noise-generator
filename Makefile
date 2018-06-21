all: noisegen

noisegen : noisegen.cpp
	g++ --std=c++11 -Wall -Wextra -Wpedantic -O3 -funroll-loops -pthread -march=native -o $@ $<

clean:
	rm -f *~ noisegen *.o

