CC = gcc
CFLAGS +=

# usage: make dump input=./libfoo.so
dump:
	readelf --dyn-syms -W ${input}

.PHONY: FORCE
FORCE:

# libfoo.so.1.x will be run forcely, no 'up to date'
libfoo.so.1.1: FORCE
	rm -f $@
	$(CC) $(CFLAGS) -c -fPIC -Wall foo_1_1.c
	$(CC) $(CFLAGS) -shared -o $@ foo_1_1.o -Wl,--version-script=foo_1_1.map -Wl,-soname,libfoo.so.1 
	rm -f libfoo.so libfoo.so.1
	ln -s $@ libfoo.so
	ln -s $@ libfoo.so.1

libfoo.so.1.2: FORCE
	rm -f $@
	$(CC) $(CFLAGS) -c -fPIC -Wall foo_1_2.c
	$(CC) $(CFLAGS) -shared -o $@ foo_1_2.o -Wl,--version-script=foo_1_2.map -Wl,-soname,libfoo.so.1 
	rm -f libfoo.so libfoo.so.1
	ln -s $@ libfoo.so
	ln -s $@ libfoo.so.1

libfoo.so.1.3: FORCE
	rm -f $@
	$(CC) $(CFLAGS) -c -fPIC -Wall foo_1_3.c
	$(CC) $(CFLAGS) -shared -o $@ foo_1_3.o -Wl,--version-script=foo_1_3.map -Wl,-soname,libfoo.so.1 
	rm -f libfoo.so libfoo.so.1
	ln -s $@ libfoo.so
	ln -s $@ libfoo.so.1

libfoo: libfoo.so.1.1 libfoo.so.1.2 libfoo.so.1.3

foo_test: foo_test.c libfoo.so.1.3
	$(CC) foo_test.o $(CFLAGS) -Wl,-rpath,`pwd` -o $@ -lfoo -L.


libnover.so.1.0: FORCE
	rm -f $@
	$(CC) $(CFLAGS) -c -fPIC -Wall no_ver.c
	$(CC) $(CFLAGS) -shared -o $@ no_ver.o -Wl,--version-script=no_ver.map -Wl,-soname,libnover.so.1 
	rm -f libnover.so libnover.so.1
	ln -s $@ libnover.so
	ln -s $@ libnover.so.1

nover_test: nover_test.c libnover.so.1.0
	$(CC) nover_test.o $(CFLAGS) -Wl,-rpath,`pwd` -o $@ -lnover -L.


libsv.so.1.1: FORCE
	rm -f $@
	$(CC) $(CFLAGS) -c -fPIC -Wall sv_v1.c
	$(CC) $(CFLAGS) -shared -o $@ sv_v1.o -Wl,--version-script=sv_v1.map -Wl,-soname,libsv.so.1 
	rm -f libsv.so libsv.so.1
	ln -s $@ libsv.so
	ln -s $@ libsv.so.1

libsv.so.1.2: FORCE
	rm -f $@
	$(CC) $(CFLAGS) -c -fPIC -Wall sv_v2.c
	$(CC) $(CFLAGS) -shared -o $@ sv_v2.o -Wl,--version-script=sv_v2.map -Wl,-soname,libsv.so.1 
	rm -f libsv.so libsv.so.1
	ln -s $@ libsv.so
	ln -s $@ libsv.so.1

sv_v1_test: sv_test.c libsv.so.1.1
	$(CC) sv_test.c $(CFLAGS) -Wl,-rpath,`pwd` -o $@ -lsv -L.

sv_v2_test: sv_test.c libsv.so.1.2
	$(CC) sv_test.c $(CFLAGS) -Wl,-rpath,`pwd` -o $@ -lsv -L.

# only clean *.o and so files
clean:
	rm -f *.o libfoo.so* libsv.so* libnover.so*

cleanall: clean
	rm -f *_test
