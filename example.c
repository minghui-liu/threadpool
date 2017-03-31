#include <stdio.h>
#include "threadpool.h"

void *print_num(void *args);

int main() {
	threadpool_t *thpool = threadpool_create(4, 4);
	threadpool_add_work(thpool, print_num, (void *)1);
	threadpool_add_work(thpool, print_num, (void *)2);
	threadpool_add_work(thpool, print_num, (void *)3);
	threadpool_add_work(thpool, print_num, (void *)4);
	threadpool_add_work(thpool, print_num, (void *)5);
	threadpool_add_work(thpool, print_num, (void *)6);
	threadpool_add_work(thpool, print_num, (void *)7);
	threadpool_add_work(thpool, print_num, (void *)8);
	sleep(3);
	threadpool_destroy(thpool);
}

void *print_num(void *args) {
	long n = (long)args;
	printf("print_num: %ld\n", n);
}
