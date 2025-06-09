
build: main.c hmap.c
	gcc main.c hmap.c -o main
	./main

test: hmap.c hmap_test.c
	gcc hmap_test.c hmap.c -o hmap_test
	./hmap_test

clean:
	rm -rf ./main ./hmap_test

.PHONY: build test clean
