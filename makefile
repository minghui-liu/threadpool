all:
	gcc -c threadpool.c -pthread
	gcc example.c threadpool.o -o example -pthread
