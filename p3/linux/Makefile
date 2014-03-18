libmem.so: mem.o
	gcc -g -lm -shared -o libmem.so mem.o -Wall -Werror

mem.o: mem.c mem.h
	gcc -g -c -fpic -o mem.o mem.c -Wall -Werror
clean:
	rm -rf mem.o libmem.so

