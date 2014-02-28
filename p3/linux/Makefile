libmem.so: mem.o
	gcc -g -shared -o libmem.so mem.o -m32 -Wall -Werror

mem.o: mem.c mem.h
	gcc -g -c -fpic -o mem.o mem.c -m32 -Wall -Werror
clean:
	rm -rf mem.o libmem.so

