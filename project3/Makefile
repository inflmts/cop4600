.PHONY: dist test mount record

wadfs/wadfs: wadfs/wadfs.cpp libWad/libWad.a
	g++ -Wall -Werror -std=c++17 -o $@ $< \
		-D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 \
		-IlibWad/ -LlibWad/ -lWad -lfuse

libWad/libWad.a: libWad/Wad.o
	ar cr $@ $^

libWad/Wad.o: libWad/Wad.cpp libWad/Wad.h
	g++ -Wall -Werror -std=c++17 -c -o $@ $<

dist: wad.tar.gz

wad.tar.gz: \
	libWad/Makefile \
	libWad/Wad.h \
	libWad/Wad.cpp \
	wadfs/Makefile \
	wadfs/wadfs.cpp
	tar -czf $@ $^

%/Makefile: Makefile
	sed '/^$*/,/^$$/!s|^|#|; t; s|$*/||g; s|libWad/|../&|g; s|-Werror||g' $< > $@

# Only specific values of $TERM will activate colors automatically.
# See:
#	https://github.com/google/googletest/issues/3455
#	https://github.com/google/googletest/issues/4355
#
test: libtest.out
	TERM=xterm ./libtest.out

libtest.out: libtest.cpp libWad/libWad.a
	g++ -g -o $@ $< -lgtest -lgtest_main -IlibWad/ -LlibWad/ -lWad

wad_dump: wad_dump.cpp libWad/libWad.a
	g++ -o $@ $< -IlibWad/ -LlibWad/ -lWad

mount: wadfs/wadfs mnt.wad mnt
	wadfs/wadfs -d mnt.wad mnt

mnt:
	mkdir $@

mnt.wad:
	cp sample1.wad $@

record:
	wf-recorder -a -f screencast.mp4
