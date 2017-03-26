/*
 * threadpool.c - An elegant threadpool implementation
 * Copyright (C) 2017 Minghui Liu
 */

#include <stdlib.h>
#include <stdio.h>

#include "threadpool.h"


int keepalive = 1;

void thread_create(thread_t *thread, jobqueue_t *jobqueue);
void thread_destroy(thread_t *thread);
void *thread_idle(void *args);
void *thread_work(void *args);
void enqueue(jobqueue_t *jobqueue, void *(*function)(void *), void *args);
void dequeue(jobqueue_t *jobqueue, job_t *job);

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
	thpool->jobqueue->front = 0;
	thpool->jobqueue->nextempty = 0;
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
		printf("creating thread %d\n", i);
		thpool->threads[i].id = i;
		thread_create(&(thpool->threads[i]), thpool->jobqueue);
		// thread_detach(&(thpool->threads[i]));
		i == num_threads ? printf("+\n") : printf("+");
	}
	printf("finished creating threads.\n");
	
	return thpool;
}

/*
 * thread_create - create one thread
 */
void thread_create(thread_t *thread, jobqueue_t *jobqueue) {
	if (pthread_create(&(thread->pthread_handle), NULL, thread_work, (void *)jobqueue)) {
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
 * thread_work - repeatedly retrieve job form job queue and work on it
 */
void *thread_work(void *pkg) {
	while (keepalive) {
		jobqueue_t *jobqueue = (jobqueue_t *)pkg;
		job_t job;
		/* get a job from job queue */
		dequeue(jobqueue, &job);
		printf("my job's number: %ld\n", (long)job.args);
		job.function(job.args);
	}
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
	keepalive = 0;
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
 * threadpool_add_work - add a jobs to the job queue
 */
int threadpool_add_work(threadpool_t *thpool, void *(*function)(void *), void *args) {
	enqueue(thpool->jobqueue, function, args);
}

/*
 * enqueue - add a job to the end of the job queue
 */
void enqueue(jobqueue_t *jobqueue, void *(*function)(void *), void *args) {
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
void dequeue(jobqueue_t *jobqueue, job_t *job) {
	pthread_mutex_lock(&(jobqueue->lock));

	/* if jobqueue is empty, wait */
	while (jobqueue->nextempty == jobqueue->front) {
		pthread_cond_wait(&(jobqueue->condvar), &(jobqueue->lock));
		if (!keepalive)
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

void *print_num(void *args) {
	long n = (long)args;
	printf("print_num: %ld\n", n);
}

int main() {
	threadpool_t *thpool = threadpool_create(4);
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
