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
	void (*function)(void *pkg);	/* function pointer */
} job_t;

typedef struct jobqueue_t {
	job_t *queue;
	int front;
	int nextempty;
	int size;
} jobqueue_t;

typedef struct threadpool_t {
	int num_threads;
	thread_t *threads;
	jobqueue_t *jobqueue;
} threadpool_t;

/* prototypes */
threadpool_t *threadpool_create(size_t num_threads);
void threadpool_destroy(threadpool_t *thpool);
void thread_create(thread_t *thread);
void thread_destroy(thread_t *thread);
void *thread_idle(void *pkg);

#endif
