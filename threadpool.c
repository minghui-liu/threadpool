/*
 * threadpool.c - An elegant threadpool implementation
 * Copyright (C) 2017 Minghui Liu
 */

#include <stdlib.h>
#include <stdio.h>

#include "threadpool.h"

/*
 * threadpool_create - create a thread pool of size threads
 */
threadpool_t *threadpool_create(size_t num_threads) {
	threadpool_t *thpool;
	
	/* allocate space for thread pool */
	thpool = (threadpool_t *)malloc(sizeof(threadpool_t));
	if (thpool == NULL) {
		fprintf(stderr, "threadpool_create(): Could not allocate memory for thread pool\n");
		return NULL;
	}
	thpool->num_threads = num_threads;

	/* allocate space for jobqueue */
	thpool->jobqueue = (jobqueue_t *)malloc(sizeof(jobqueue_t));
	thpool->jobqueue->size = 16; 
	thpool->jobqueue->queue = (job_t *)malloc(thpool->jobqueue->size * sizeof(job_t));

	/* allocate space for threads */
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

/*
 * thread_create - create one thread
 */
void thread_create(thread_t *thread) {
	if (pthread_create(&(thread->pthread_handle), NULL, thread_idle, (void *)thread->id)) {
		fprintf(stderr, "thread_create(): Could not create thread\n");
	}
}

/*
 * thread_idle - the function that threads run when there is no work to do
 */
void *thread_idle(void *pkg) {
	long id = (long)pkg;
	printf("thread %ld finished\n", id);
}

/*
 * thread_destroy - destroy a single thread
 */
void thread_destroy(thread_t *thread) {
	pthread_join(thread->pthread_handle, NULL);
	printf("joined thread %d\n", thread->id);
}

/*
 * threadpool_destroy - wait for threads to finish their work and destroy 
 *   the thread pool
 */
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
