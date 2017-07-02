all: build

build: clean main

examples: basic

basic: basic.cpp
	clang++ -std=c++11 basic.cpp ../src/field_t.cpp ../src/group_t.cpp ../src/board_t.cpp -I ../src -o basic

ladder_capture: ladder_capture.cpp
	clang++ -g -std=c++11 ladder_capture.cpp ../src/field_t.cpp ../src/group_t.cpp ../src/board_t.cpp ../src/sgfbin.cpp -I ../src -o ladder_capture


clean:
	rm -f *.o *.so *.pyc *.npy