all: server

main: server.cpp
	g++ server.cpp -o server
	./server

clean:
	rm -f server