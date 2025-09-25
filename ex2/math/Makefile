.PHONY: clean

libmath.a: mean.o
	ar cr libmath.a mean.o
mean.o: mean.c
	cc -c mean.c
clean:
	rm -f *.a *.o
