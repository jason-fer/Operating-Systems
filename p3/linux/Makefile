libmem.so: mem.o
	gcc -g -m32 -shared -o libmem.so mem.o
	false	#fail the build test since no makefile was turned in

mem.o: mem.c mem.h
	gcc -g -m32 -c -fpic -o mem.o mem.c
    
clean:
	rm -rf mem.o libmem.so

