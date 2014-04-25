CC   = gcc
OPTS = -Wall -Werror

all: clean libmfs server client

libmfs: mfs.o udp.o
	$(CC) -lm -shared -o libmfs.so mfs.o udp.o $(OPTS)

server: server.o udp.o mfs.o
	$(CC) -o server server.o udp.o mfs.o $(OPTS)

client: client.o udp.o mfs.o
	$(CC) -o client client.o udp.o mfs.o $(OPTS)

clean:
	rm -f server.o udp.o client.o server client libmfs.so mfs.o