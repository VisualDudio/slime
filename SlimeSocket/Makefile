CXX = /opt/rh/devtoolset-8/root/usr/bin/gcc

all: socket.c indexer.c
	$(CXX) -c -g -O3 -Wall -fPIC socket.c indexer.c
	$(CXX) -g -shared -o SlimSocket.so socket.o indexer.o -ldl

debug: socket.c indexer.c
	$(CXX) -c -g -DSLIM_DEBUG -O3 -Wall -fPIC preload.c indexer.c
	$(CXX) -g -shared -o SlimSocket.so socket.o indexer.o -ldl

clean:
	rm -f *.so *.o
