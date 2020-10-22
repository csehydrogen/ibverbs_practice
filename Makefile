CXXFLAGS=-Wall
LDLIBS=-libverbs -lrdmacm

all: server client test

server: util.o

client: util.o

run_server: server
	./server

run_client: client
	./client

run_test: test
	./test

clean:
	rm -rf server client test util.o
