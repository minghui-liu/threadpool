/*
 * threadpool.c - An elegant threadpool implementation
 * Copyright (C) 2017 Minghui Liu
 */

#include <stdlib.h>
#include <stdio.h>

#include "threadpool.h"

void thread_create(thread_t *thread);
void thread_destroy(thread_t *thread);
void *thread_idle(void *pkg);


/*
 * threadpool_create - create a thread pool of size threads
 */
threadpool_t *threadpool_create(size_t num_threads) {
	threadpool_t *thpool;
	
	thpool = (threadpool_t *)malloc(sizeof(threadpool_t));
	if (thpool == NULL) {
		fprintf(stderr, "threadpool_create(): Could not allocate memory for thread pool\n");
		return NULL;
	}
	thpool->num_threads = num_threads;
	
	/* allocate space for pthreads */
	thpool->threads = (thread_t *)malloc(num_threads * sizeof(thread_t));
	if (thpool->threads == NULL) {
		fprintf(stderr, "threadpool_create(): Could not allocate memory for threads\n");
		return NULL;
	}

	printf("creating pthreads ...\n");
	/* create threads in pool */
	int i;
	for (i = 0; i < thpool->num_threads; i++) {
		printf("creating thread %d\n", i);
		thpool->threads[i].id = i;
		thread_create(&(thpool->threads[i]));
		// thread_detach(&(thpool->threads[i]));
		i == num_threads ? printf("+\n") : printf("+");
	}
	printf("finished creating threads.\n");
	
	return thpool;
}

void thread_create(thread_t *thread) {
	if (pthread_create(&(thread->pthread_handle), NULL, thread_idle, (void *)thread->id)) {
		fprintf(stderr, "thread_create(): Could not create thread\n");
	}
}

void *thread_idle(void *pkg) {
	long id = (long)pkg;
	printf("thread %ld finished\n", id);
}

void thread_destroy(thread_t *thread) {
	pthread_join(thread->pthread_handle, NULL);
	printf("joined thread %d\n", thread->id);
}

void threadpool_destroy(threadpool_t *thpool) {
	int i;
	for (i = 0; i < thpool->num_threads; i++) {
		thread_destroy(&(thpool->threads[i]));
	}
	printf("finished joining threads\n");
	free(thpool->threads);
	free(thpool);

}

int main() {
	threadpool_t *thpool = threadpool_create(4);
	threadpool_destroy(thpool);
}
