#ifndef FOO_THREAD_H
#define FOO_THREAD_H


// Maximum number of threads a process can create
#define FOOTHREAD_THREADS_MAX 10

// Default stack size for each thread
#define FOOTHREAD_DEFAULT_STACK_SIZE (2 * 1024 * 1024) // 2MB

// Thread state constants
#define FOOTHREAD_JOINABLE 0
#define FOOTHREAD_DETACHED 1

// Data type for foothread
// Define a placeholder type for thread ID
typedef int foothread_id_t;

// Data type for foothread
typedef struct {
    foothread_id_t tid;                 // Thread ID
    int state;                          // Thread state (joinable or detached)
    void *(*start_routine)(void *);     // Pointer to the start routine
    void *arg;                          // Pointer to the argument for the start routine
} foothread_t;

typedef struct {
    int join_type;     // Join type: FOOTHREAD_JOINABLE or FOOTHREAD_DETACHED
    int stack_size;    // Stack size in bytes
} foothread_attr_t;



// Function to create a new thread
void foothread_create(foothread_t *thread, foothread_attr_t *attr, void *(*start_routine)(void *), void *arg);
// Function to join a joinable thread
int foothread_join(foothread_t *thread);

// Function to detach a thread
int foothread_detach(foothread_t *thread);

// Function to cancel a thread
int foothread_cancel(foothread_t *thread);

#include <semaphore.h>

// Define the foothread mutex data type
// Define the foothread mutex data type
typedef struct {
    sem_t semaphore; // Binary semaphore to act as the mutex
    int locked;      // Flag to indicate if the mutex is locked
    pid_t owner;     // PID of the thread that currently owns the mutex
} foothread_mutex_t;

// Function to initialize the mutex
void foothread_mutex_init(foothread_mutex_t *mutex);

// Function to lock the mutex
void foothread_mutex_lock(foothread_mutex_t *mutex);

// Function to unlock the mutex
int foothread_mutex_unlock(foothread_mutex_t *mutex);

// Function to destroy the mutex
void foothread_mutex_destroy(foothread_mutex_t *mutex);

// Define the foothread barrier data type
typedef struct {
    sem_t mutex;        // Semaphore to protect shared variables
    sem_t barrier;      // Semaphore to wait for threads to arrive
    int count;          // Count of threads that have arrived at the barrier
    int max_count;      // Total number of threads required to wait at the barrier
} foothread_barrier_t;

// Function prototypes for barrier functions

// Initialize a foothread barrier
void foothread_barrier_init(foothread_barrier_t *barrier, int count);

// Wait for all threads to arrive at the barrier
void foothread_barrier_wait(foothread_barrier_t *barrier);

// Destroy a foothread barrier
void foothread_barrier_destroy(foothread_barrier_t *barrier);


sem_t exit_barrier_sem;

sem_t exit_barrier_sem1;
foothread_barrier_t exit_barrier;
sem_t num_joinable_threads_mutex;

#endif /* FOO_THREAD_H */
