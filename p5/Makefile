CC   = gcc
OPTS = -Wall -Werror

all: clean server client

# this generates the target executables
server: server.o udp.o
	$(CC) -o server server.o udp.o 

client: client.o udp.o
	$(CC) -o client client.o udp.o 

# this is a generic rule for .o files 
%.o: %.c 
	$(CC) $(OPTS) -c $< -o $@

clean:
	rm -f server.o udp.o client.o server client libmfs.so

#The interface that the library supports is defined in mfs.h . The library should be called libmfs.so