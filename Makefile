all: main test.so

test: all
	./main test.so

main: main.c
	gcc -g -Wall -O0 -o $@ $< -ldl -lrt

test.so: test.c
	gcc -g -Wall -o $@ $< -fPIC -shared

clean:
	rm -f main test.so
