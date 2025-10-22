.PHONY: run

run: part1 part2 part3

part1.out: li_part2.cpp
	g++ -std=c++17 -Wall -Werror -o $@ $< -DPART1
part1: part1.out
	@printf '\033[1;32mreptilian@localhost\033[0m:\033[1;34m~/ex7\033[0m$$ make part1\n'
	./part1.o | ./part1.out
	@sleep 1
	./part1.o | ./part1.out
	@sleep 1

part2.out: li_part2.cpp
	g++ -std=c++17 -Wall -Werror -o $@ $<
part2: part2.out
	@printf '\033[1;32mreptilian@localhost\033[0m:\033[1;34m~/ex7\033[0m$$ make part2\n'
	@rm -f part2.fifo
	mkfifo part2.fifo
	./part1.o > part2.fifo &
	./part2.out < part2.fifo
	@sleep 1
	./part1.o > part2.fifo &
	./part2.out < part2.fifo
	@sleep 1

part3.out: li_part3.cpp
	g++ -std=c++17 -Wall -Werror -o $@ $<
part3: part3.out
	@printf '\033[1;32mreptilian@localhost\033[0m:\033[1;34m~/ex7\033[0m$$ make part3\n'
	./part3.out 42 15 8 16 23
