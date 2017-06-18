all: build

build: clean main

main: main.cc
	clang++ -std=c++11 main.cc -o main

clean:
	rm -f *.o *.so *.pyc *.npy