CC = gcc
LD = ld

a.out: file1.o file2.o
	$(LD) -e 0 file1.o file2.o

file1.o: file1.c
	$(CC) -c file1.c

file2.o: file2.c
	$(CC) -c file2.c

# F=<the file to be checked>
check:
	-readelf -SW $(F)
	-readelf -s $(F)

# S=<name of section>
# F=<the file to be dumped>
dump:
	-objdump --full-contents --section=$(S) $(F)

clean:
	rm -f *.o a.out

DIRS = symbols 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19

cleanall:
	for dir in $(DIRS); do $(MAKE) -C $$dir clean || exit "$$?"; done	