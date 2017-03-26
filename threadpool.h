/*
 * threadpool.h
 * Copyright (C) 2017 Minghui Liu
 */
#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <pthread.h>

/* structure */
typedef struct thread_t {
	int id;
	pthread_t pthread_handle;
} thread_t;

typedef struct job_t {
	void *(*function)(void *pkg);	/* function pointer */
	void *args;						/* arguments */
} job_t;

typedef struct jobqueue_t {
	job_t *queue;
	int front;
	int nextempty;
	int size;
	pthread_mutex_t lock;
	pthread_cond_t condvar;
} jobqueue_t;

typedef struct threadpool_t {
	int num_threads;
	thread_t *threads;
	jobqueue_t *jobqueue;

} threadpool_t;

/* prototypes */
threadpool_t *threadpool_create(size_t num_threads);
void threadpool_destroy(threadpool_t *thpool);

#endif
