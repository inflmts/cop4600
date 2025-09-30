.PHONY: test
ex5.out: threads.cpp
	g++ $< -o $@ -pthread -std=c++11
test: ex5.out
	nice -n 19 ./ex5.out 1414
	nice -n 19 ./ex5.out 1414
