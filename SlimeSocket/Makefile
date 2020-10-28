CXX = /usr/bin/g++
SRC = socket.c msg.c
OBJ = socket.o msg.o

all: $(SRC)
	$(CXX) -c -g -O3 -Wall -fPIC $(SRC)
	$(CXX) -g -shared -o SlimeSocket.so $(OBJ) -ldl

debug: $(SRC)
	$(CXX) -c -g -DSLIM_DEBUG -O3 -Wall -fPIC $(SRC)
	$(CXX) -g -shared -o SlimeSocket.so $(OBJ) -ldl

clean:
	rm -f *.so *.o
