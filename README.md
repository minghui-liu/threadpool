[![Build Status](https://travis-ci.org/minghui-liu/threadpool.svg?branch=master)](https://travis-ci.org/minghui-liu/threadpool)

# An elegant POSIX compatible thread pool implementation in C

## Features
* Simple and clean API
* ANSI C and POSIX compatible
* Small filesize, just two files, easy to add to your project
* Well documented and readable code, easy to understand and to modify

## Usage
First of all, include threadpool.h in your project.
```
include "threadpool.h"
```
Then, create a threadpool with some number of threads.
```
threadpool_t *pool = threadpool_create(4);
```
Now you can add jobs to the threadpool.
```
threadpool_add_work(pool, some_job, args);
```
Finally, destroy the threadpool after you are done.
```
threadpool_destroy(pool);
```

## Compilation
This project is not precompiled. You should add both threadpool.c and threadpool.h to your project and compile them with your project. Don't forget to add -pthread when you compile this project since it uses pthreads. 
```
gcc -c threadpool.c -pthread
gcc myproject.o threadpool.o -o myproject
```

## Contribute
If you like this project and find it useful, please help me make it better. Any contribution is welcomed and appreciated.
