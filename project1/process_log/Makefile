libprocess_log.a: process_log.o
	ar cr $@ $^
process_log.o: process_log.h
	gcc -c -DPROCESS_LOG_IMPLEMENTATION -x c -o $@ $<
