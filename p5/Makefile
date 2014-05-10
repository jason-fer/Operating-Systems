CC   = gcc
OPTS = -g -Wall -Werror -ggdb

all: clean libmfs server client

# Dump everything into a shared library
libmfs: mfs.c mfs.h
	$(CC) -c -fpic mfs.c $(OPTS)
	$(CC) -c -fpic udp.c $(OPTS)
	$(CC) -lm -shared -o libmfs.so mfs.o udp.o $(OPTS)

# Now we only need to link up the shared library.
server: server.o udp.o mfs.o
	$(CC) -o server server.o -L. libmfs.so $(OPTS)

client: client.o udp.o mfs.o
	$(CC) -o client client.o -L. libmfs.so $(OPTS)

clean:
	rm -f server.o udp.o client.o server client libmfs.so mfs.o
