CXXFLAGS=-Wall
LDLIBS=-libverbs -lrdmacm -lpthread

all: apm

run_server: apm
	./apm -s

run_client: apm
	./apm -a palace_ib

clean:
	rm -rf apm
