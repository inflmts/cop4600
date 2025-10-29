.PHONY: dist

wallserver: wallserver.c
	gcc -Wall -o $@ $<

dist: ex8.tar.gz

ex8.tar.gz: Makefile wallserver.c
	tar -czvf ex8.tar.gz -C .. ex8/Makefile ex8/wallserver.c
