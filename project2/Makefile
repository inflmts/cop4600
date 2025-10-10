.PHONY: test dist

MemoryManager/libMemoryManager.a: MemoryManager/MemoryManager.o
	ar cr $@ $<

MemoryManager/MemoryManager.o: MemoryManager/MemoryManager.cpp
	g++ -std=c++17 -c -o $@ $<

test: test.out
	./test.out

test.out: test.cpp MemoryManager/libMemoryManager.a
	g++ -std=c++17 -o $@ $< -L MemoryManager -lMemoryManager

dist: MemoryManager.tgz

MemoryManager.tgz: \
	MemoryManager/MemoryManager.h \
	MemoryManager/MemoryManager.cpp \
	MemoryManager/Makefile
	tar -czvf $@ $^
