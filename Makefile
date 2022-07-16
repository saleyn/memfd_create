all: main test.so

test: all
	./main test.so

main: main.c
	gcc -g -O0 -o $@ $< -lcurl

test.so: test.c
	gcc -g -o $@ $< -fPIC -shared

clean:
	rm -f main test.so
