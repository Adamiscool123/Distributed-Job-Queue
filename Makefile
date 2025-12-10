all: main

main: main.cpp
	g++ main.cpp -o main
	./main

clean:
	rm -f main