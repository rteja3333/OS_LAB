#define _GNU_SOURCE // Required for using gettid()
#include "foothread.h"
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syscall.h>
#include <bits/sched.h>
#include <linux/sched.h>
#include <semaphore.h>
#include <errno.h>



// Function to initialize the mutex
void foothread_mutex_init(foothread_mutex_t *mutex) {
    // Initialize the semaphore with value 1 (unlocked)

    sem_init(&(mutex->semaphore), 0, 1);
    mutex->locked = 0;
    mutex->owner = -1; // No thread owns the mutex initially
    // printf("mutex initialised\n");
}

// Function to lock the mutex
void foothread_mutex_lock(foothread_mutex_t *mutex) {
    // Wait on the semaphore
    sem_wait(&(mutex->semaphore));
    // Set the owner to the current thread ID (TID)
    mutex->owner = gettid();
    // Mark the mutex as locked
    mutex->locked = 1;
}

// Function to unlock the mutex
int foothread_mutex_unlock(foothread_mutex_t *mutex) {
    // Check if the calling thread is the owner of the mutex
    if (mutex->owner != gettid()) {
        // printf("mutex trying to unlock by others\n");
        // Error: Only the locking thread can unlock the mutex
        errno = EPERM;
        return -1;
    }
    // Check if the mutex is currently locked
    if (mutex->locked == 0) {
        // Error: Attempt to unlock an unlocked mutex
        errno = EINVAL;
        return -1;
    }
    // printf("releasing the mutex\n");
    // Release the semaphore
    sem_post(&(mutex->semaphore));
    // Mark the mutex as unlocked
    mutex->locked = 0;
    return 0;
}

// Function to destroy the mutex
void foothread_mutex_destroy(foothread_mutex_t *mutex) {
    // Destroy the semaphore
    sem_destroy(&(mutex->semaphore));
}





// Function to initialize the barrier
void foothread_barrier_init(foothread_barrier_t *barrier, int count) {
    // Initialize the semaphore to protect shared variables with value 1
    sem_init(&(barrier->mutex), 0, 1);
    // Initialize the semaphore for the barrier with value 0
    sem_init(&(barrier->barrier), 0, 0);
    barrier->count = 0;
    barrier->max_count = count;
        // printf("barrier initialised with %d\n",count);

}

// Function for threads to wait at the barrier
void foothread_barrier_wait(foothread_barrier_t *barrier) {
    // Check if the barrier is initialized
    if (barrier == NULL) {
        // Print an error message or handle the error appropriately
        fprintf(stderr, "Error: Barrier is not initialized\n");
        return; // Return from the function
    }
    // printf("entered barrier wait\n");
    // Lock the mutex to protect the shared count variable
    sem_wait(&(barrier->mutex));
    barrier->count++;
    // printf("count:%d\n",barrier->count);
    // Check if all threads have arrived at the barrier
    if (barrier->count == barrier->max_count) {
        // printf("releasing all barriers\n");
        // Release all waiting threads
        for (int i = 0; i < barrier->max_count-1 ; ++i) {
            sem_post(&(barrier->barrier));
        }
        // Reset count for the next barrier
        barrier->count = 0;
        
        // Unlock the mutex
        sem_post(&(barrier->mutex));
    } else {
        // Unlock the mutex
        sem_post(&(barrier->mutex));
        // Wait at the barrier until all threads have arrived
        sem_wait(&(barrier->barrier));
    }
}

// Function to destroy the barrier
void foothread_barrier_destroy(foothread_barrier_t *barrier) {
    // Check if the barrier is initialized
    if (barrier == NULL) {
        // Print an error message or handle the error appropriately
        fprintf(stderr, "Error: Barrier is not initialized\n");
        return; // Return from the function
    }

    // Destroy the mutex semaphore
    sem_destroy(&(barrier->mutex));
    // Destroy the barrier semaphore
    sem_destroy(&(barrier->barrier));
}



// Default stack size for each thread
#define FOOTHREAD_DEFAULT_STACK_SIZE (2 * 1024 * 1024) // 2MB

// Shared variable for the number of joinable threads
static int num_joinable_threads = 0;

// Flag to track initialization status
static int num_joinable_threads_mutex_initialized = 0;

// Initialize the semaphore
void init_num_joinable_threads_mutex() {
    // Ensure initialization is done only once
    if (!num_joinable_threads_mutex_initialized) {
        sem_init(&num_joinable_threads_mutex, 0, 1);
        num_joinable_threads_mutex_initialized = 1;
    }
}

// Function to increment num_joinable_threads
void increment_num_joinable_threads() {
    sem_wait(&num_joinable_threads_mutex);
    num_joinable_threads++;
    sem_post(&num_joinable_threads_mutex);
}

// Function to decrement num_joinable_threads
void decrement_num_joinable_threads() {
    sem_wait(&num_joinable_threads_mutex);
    num_joinable_threads--;
    sem_post(&num_joinable_threads_mutex);
}

// Default attributes initializer
#define FOOTHREAD_ATTR_INITIALIZER { FOOTHREAD_JOINABLE, FOOTHREAD_DEFAULT_STACK_SIZE }



void foothread_create(foothread_t *thread, foothread_attr_t *attr, void *(*start_routine)(void *), void *arg) {
    char *stack;
    char *stack_top;
    pid_t pid;

    // Initialize attributes if not provided
    if (attr == NULL) {
        foothread_attr_t default_attr = FOOTHREAD_ATTR_INITIALIZER;
        attr = &default_attr;
    }

    stack = malloc(attr->stack_size);
    if (stack == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    stack_top = stack + attr->stack_size;

    // Create the thread-like process
    pid = clone(start_routine, stack_top, CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD, arg);
    if (pid == -1) {
        perror("clone");
        free(stack);
        exit(EXIT_FAILURE);
    }

    thread->tid = pid;
    thread->state = attr->join_type; // Set the thread state based on join type
    if((attr->join_type & FOOTHREAD_JOINABLE)==FOOTHREAD_JOINABLE){
        init_num_joinable_threads_mutex();
        increment_num_joinable_threads();
        // printf("no.of.joinable ++:%d\n",num_joinable_threads);
    }

}

// Function to set the join type attribute
void foothread_attr_setjointype(foothread_attr_t *attr, int join_type) {
    attr->join_type = join_type;
}

// Function to set the stack size attribute
void foothread_attr_setstacksize(foothread_attr_t *attr, int stack_size) {
    attr->stack_size = stack_size;
}

// Define a flag to track whether the mutex has been initialized
static int exit_barrier_mutex_initialized = 0;

// Initialize the exit barrier mutex
void foothread_init_exit_barrier_mutex() {
    // Ensure the mutex is initialized only once
    if (!exit_barrier_mutex_initialized) {
        // printf("exit barrier mutex initialised\n");
            sem_init(&num_joinable_threads_mutex, 0, 1);
//initialised sem for using mutex for no.of joinble threads
        sem_init(&(exit_barrier_sem), 0, 0); // Initialize the mutex with value 1
        sem_init(&(exit_barrier_sem1), 0, 0); // Initialize the mutex with value 1

        exit_barrier_mutex_initialized = 1; // Set the flag to indicate initialization
    }
}

void handle_mutex_unlock_error(int result) {
    if (result == -1) {
        if (errno == EPERM) {
            fprintf(stderr, "Error: Only the locking thread can unlock the mutex\n");
        } else {
            fprintf(stderr, "Error unlocking mutex: %s\n", strerror(errno));
        }
        exit(EXIT_FAILURE);
    }
}


// Function to exit a thread
void foothread_exit() {
    pid_t tid = syscall(SYS_gettid); // Get the TID of the current thread
    pid_t pid = getpid(); // Get the PID of the process

    // Initialize the exit barrier mutex if it hasn't been initialized
    foothread_init_exit_barrier_mutex();

    // Check if the current thread is the leader thread
    if (tid == pid) {
        // Leader thread
        // printf("leader thread called exit\n");
        // Initialize the exit barrier with num_joinable_threads

        sem_wait(&num_joinable_threads_mutex);

        // Initialize the exit barrier with num_joinable_threads
        foothread_barrier_init(&exit_barrier, num_joinable_threads);

            // Release the mutex after initialization
            sem_post(&num_joinable_threads_mutex); 
        // printf("no.of threads:%d\n",num_joinable_threads);
        // printf("leader thread initialised the exit_barrier\n");
        // Signal the mutex to indicate the creation of the exit barrier
        sem_post(&(exit_barrier_sem));
        // printf("leader thread unlocked  the exit_barrier\n");

        // Wait for all joinable threads to terminate
        // foothread_barrier_wait(&exit_barrier);
        sem_wait(&(exit_barrier_sem1));

        // All joinable threads have terminated, now terminate the leader thread
        // printf("Leader thread with PID %d is terminating\n", pid);
        // You can perform additional cleanup or bookkeeping here if needed
        // Terminate the leader thread
        // exit(0);
    } else {
                // printf("follower thread called exit\n");


        // if (thread.state == FOOTHREAD_DETACHED) {
        //     foothread_mutex_unlock(&exit_barrier_mutex);
        // }
        // Follower thread
        // Wait on the mutex to be released by the leader
        sem_wait(&(exit_barrier_sem));
        // printf("follower thread unlocked the exit mutext after barrier created \n");
        sem_post(&(exit_barrier_sem));
        // Wait on the exit barrier
        foothread_barrier_wait (&exit_barrier);
        sem_post(&(exit_barrier_sem1));

        // printf("Follower thread with TID %d is terminating\n", tid);

        // Terminate the follower thread
        // exit(0);
    }
}