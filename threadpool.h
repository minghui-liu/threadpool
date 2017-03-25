/*
 * threadpool.h
 * Copyright (C) 2017 Minghui Liu
 */
#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <pthread.h>

typedef struct thread_t {
	int id;
	pthread_t pthread_handle;
} thread_t;

typedef struct threadpool_t {
	int num_threads;
	thread_t *threads;

} threadpool_t;


#endif
