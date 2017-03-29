/*
 * MIT License
 *
 * Copyright (c) 2017 Minghui Liu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <pthread.h>

/* type definitions */
typedef struct threadpool_t threadpool_t;
typedef struct jobqueue_t jobqueue_t;
typedef struct thread_t thread_t;
typedef struct job_t job_t;

/* structure */
struct thread_t {
	int id;
	pthread_t pthread_handle;
};

struct job_t {
	void *(*function)(void *pkg);	/* function pointer */
	void *args;						/* arguments */
};

struct jobqueue_t {
	job_t *queue;
	int front;
	int nextempty;
	int size;
	pthread_mutex_t lock;
	pthread_cond_t condvar;
	threadpool_t *thpool;
};

struct threadpool_t {
	int num_threads;
	thread_t *threads;
	jobqueue_t *jobqueue;
	int keepalive;
};

/* prototypes */
threadpool_t *threadpool_create(size_t num_threads);
void threadpool_destroy(threadpool_t *thpool);
int threadpool_add_work(threadpool_t *thpool, void *(*function)(void *), void *args);

#endif
