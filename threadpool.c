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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "threadpool.h"


static void create_worker(threadpool_t *thpool, thread_t *thread);
static void destroy_worker(thread_t *thread);
static void *worker_thread(void *args);
static void notify_waiters(threadpool_t *thpool);
static void enqueue(jobqueue_t *jobqueue, void *(*function)(void *), void *args);
static void dequeue(jobqueue_t *jobqueue, job_t *job);

static sigset_t fillset;	

/*
 * threadpool_create - create a thread pool of size threads
 */
threadpool_t *threadpool_create(size_t min_threads, size_t max_threads, size_t jobqueue_size) {
	/* check if min_threads and max_threads are valid */
	if (min_threads < 0 || max_threads <= 0 || min_threads > max_threads) {
		// invalid, should error and return NULL
		fprintf(stderr, "threadpool_create(): invalid min and max thread numbers\n");
		return NULL;
	}

	threadpool_t *thpool;
	sigfillset(&fillset);

	/* allocate space for thread pool */
	thpool = (threadpool_t *)malloc(sizeof(threadpool_t));
	if (thpool == NULL) {
		fprintf(stderr, "threadpool_create(): Could not allocate memory for thread pool\n");
		return NULL;
	}
	thpool->keepalive = 1;

	/* allocate space for jobqueue */
	thpool->jobqueue = (jobqueue_t *)malloc(sizeof(jobqueue_t));
	thpool->jobqueue->size = jobqueue_size; 
	thpool->jobqueue->queue = (job_t *)malloc(thpool->jobqueue->size * sizeof(job_t));
	thpool->jobqueue->front = 0;
	thpool->jobqueue->nextempty = 0;
	thpool->jobqueue->thpool = thpool;
	pthread_mutex_init(&(thpool->jobqueue->lock), NULL);
	pthread_cond_init(&(thpool->jobqueue->condvar), NULL);

	/* create min number of threads and put them in a doubly linkedlist */
	thpool->threads_head = thpool->threads_tail = NULL;		// in case min_threads == 0
	int i;
	for (i = 0; i < min_threads; i++) {
		thread_t *temp = (thread_t *)malloc(sizeof(thread_t));
		create_worker(thpool, temp);
		if (temp == NULL) {
			fprintf(stderr, "threadpool_create(): Could not allocate memory for threads\n");
			return NULL;
		}
		if (i == 0) {
			thpool->threads_head = thpool->threads_tail = temp;
		} else {	
			// attach to the end of linkedlist
			thpool->threads_tail->next = temp;
			temp->prev = thpool->threads_tail;
			thpool->threads_tail = temp;
		}
	}

	thpool->num_threads = min_threads;
	// TODO: move thpool->keepalive = 1 here
	// start workers here
	// or maybe let the user start threadpool
	
	return thpool;
}

//
// TODO: add methods to dynamically chang the nunmber of threads
//
/*
 * threadpool_adjust - adjust the number of threads in threadpool
 *   returns 1 if successful otherwise returns 0
 */
int threadpool_adjust(threadpool_t *thpool, size_t min_threads, size_t max_threads) {
	// TODO: implement thread number adjustment
	return 0;
}

/*
 * threadpool_add_work - add a jobs to the job queue
 */
int threadpool_add_work(threadpool_t *thpool, void *(*function)(void *), void *args) {
	enqueue(thpool->jobqueue, function, args);
}

/*
 * threadpool_destroy - wait for threads to finish their work and destroy 
 *   the thread pool
 */
void threadpool_destroy(threadpool_t *thpool) {
	thpool->keepalive = 0;
	notify_waiters(thpool);

	/* destroy all threads, from tail forward */
	thread_t *runner = thpool->threads_tail;
	while (runner) {
		destroy_worker(runner);
		thread_t *prev = runner->prev;
		free(runner);
		runner = prev;
	}
	thpool->threads_head = thpool->threads_tail = NULL;

	free(thpool->jobqueue->queue);
	free(thpool->jobqueue);
	free(thpool);
}

static void notify_waiters(threadpool_t *thpool) {
	/* wake up all waiting threads */
	pthread_cond_broadcast(&(thpool->jobqueue->condvar));
}

// TODO: return error code if error()
/*
 * create_worker - create a worker thread
 */
static void create_worker(threadpool_t *thpool, thread_t *thread) {
	sigset_t original_set;
	pthread_sigmask(SIG_SETMASK, &fillset, &original_set);
	if (pthread_create(&(thread->pthread_handle), NULL, worker_thread, (void *)thpool)) {
		fprintf(stderr, "create_worker(): Could not create thread\n");
	}
	pthread_sigmask(SIG_SETMASK, &original_set, NULL);
}

/*
 * worker_thread - repeatedly retrieve job form job queue and work on it
 */
static void *worker_thread(void *args) {
	threadpool_t *thpool = (threadpool_t *)args;
	jobqueue_t *jobqueue = thpool->jobqueue;
	job_t job;
	// TODO: change condition here
	// while i am idle for longer than threshhold and nthreads > min
	// i can therminate
	while (thpool->keepalive) {
		/* get a job from job queue */
		dequeue(jobqueue, &job);
		/* execute it */
		job.function(job.args);
	}
}

/*
 * destroy_worker - destroy a single worker thread
 */
static void destroy_worker(thread_t *thread) {
	pthread_join(thread->pthread_handle, NULL);
}

/*
 * enqueue - add a job to the end of the job queue
 */
static void enqueue(jobqueue_t *jobqueue, void *(*function)(void *), void *args) {
	pthread_mutex_lock(&(jobqueue->lock));
	/* if jobqueue is full, wait */
	while (jobqueue->nextempty - jobqueue->front == jobqueue->size) {
		pthread_cond_wait(&(jobqueue->condvar), &(jobqueue->lock));
	}

	/* critical section, add new job */
	jobqueue->queue[jobqueue->nextempty % jobqueue->size].function = function;
	jobqueue->queue[jobqueue->nextempty % jobqueue->size].args = args;
	jobqueue->nextempty++;

	/* wake up waiting threads */
	pthread_cond_broadcast(&(jobqueue->condvar));

	pthread_mutex_unlock(&jobqueue->lock);
}

/*
 * dequeue - return a job from front of the job queue
 */
static void dequeue(jobqueue_t *jobqueue, job_t *job) {
	pthread_mutex_lock(&(jobqueue->lock));

	/* if jobqueue is empty, wait */
	while (jobqueue->nextempty == jobqueue->front) {
		pthread_cond_wait(&(jobqueue->condvar), &(jobqueue->lock));
		if (!(jobqueue->thpool->keepalive)) {
			pthread_mutex_unlock(&(jobqueue->lock));
			pthread_exit(0);
		}
	}

	/* critical section, remove a job */
	job->function = jobqueue->queue[jobqueue->front % jobqueue->size].function;
	job->args = jobqueue->queue[jobqueue->front % jobqueue->size].args;
	jobqueue->front++;

	/* wake up waiting threads */
	pthread_cond_broadcast(&(jobqueue->condvar));

	pthread_mutex_unlock(&(jobqueue->lock));
}


