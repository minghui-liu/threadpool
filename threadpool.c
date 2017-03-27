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

#include "threadpool.h"


static void thread_create(threadpool_t *thpool, thread_t *thread);
static void thread_destroy(thread_t *thread);
static void *thread_work(void *args);
static void enqueue(jobqueue_t *jobqueue, void *(*function)(void *), void *args);
static void dequeue(jobqueue_t *jobqueue, job_t *job);

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
	thpool->keepalive = 1;

	/* allocate space for jobqueue */
	thpool->jobqueue = (jobqueue_t *)malloc(sizeof(jobqueue_t));
	thpool->jobqueue->size = 16; 
	thpool->jobqueue->queue = (job_t *)malloc(thpool->jobqueue->size * sizeof(job_t));
	thpool->jobqueue->front = 0;
	thpool->jobqueue->nextempty = 0;
	thpool->jobqueue->thpool = thpool;
	pthread_mutex_init(&(thpool->jobqueue->lock), NULL);
	pthread_cond_init(&(thpool->jobqueue->condvar), NULL);

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
		// printf("creating thread %d\n", i);
		thpool->threads[i].id = i;
		thread_create(thpool, &(thpool->threads[i]));
		// thread_detach(&(thpool->threads[i]));
		i == num_threads-1 ? printf("+\n") : printf("+");
	}
	printf("finished creating threads.\n");
	
	return thpool;
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
	/* wake up all waiting threads */
	pthread_cond_broadcast(&(thpool->jobqueue->condvar));

	/* destroy all threads */
	int i;
	for (i = 0; i < thpool->num_threads; i++) {
		thread_destroy(&(thpool->threads[i]));
	}
	printf("finished joining threads\n");

	free(thpool->threads);
	free(thpool->jobqueue->queue);
	free(thpool->jobqueue);
	free(thpool);
}

/*
 * thread_create - create one thread
 */
static void thread_create(threadpool_t *thpool, thread_t *thread) {
	if (pthread_create(&(thread->pthread_handle), NULL, thread_work, (void *)thpool)) {
		fprintf(stderr, "thread_create(): Could not create thread\n");
	}
}

/*
 * thread_work - repeatedly retrieve job form job queue and work on it
 */
static void *thread_work(void *args) {
	threadpool_t *thpool = (threadpool_t *)args;
	jobqueue_t *jobqueue = thpool->jobqueue;
	job_t job;
	while (thpool->keepalive) {
		/* get a job from job queue */
		dequeue(jobqueue, &job);
		/* execute it */
		job.function(job.args);
	}
}

/*
 * thread_destroy - destroy a single thread
 */
static void thread_destroy(thread_t *thread) {
	pthread_join(thread->pthread_handle, NULL);
	printf("joined thread %d\n", thread->id);
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
		if (!(jobqueue->thpool->keepalive))
			exit(0);
	}

	/* critical section, remove a job */
	job->function = jobqueue->queue[jobqueue->front % jobqueue->size].function;
	job->args = jobqueue->queue[jobqueue->front % jobqueue->size].args;
	jobqueue->front++;

	/* wake up waiting threads */
	pthread_cond_broadcast(&(jobqueue->condvar));

	pthread_mutex_unlock(&(jobqueue->lock));
}


