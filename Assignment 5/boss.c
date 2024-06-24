#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_NODES 100

// Function to print error messages and exit
void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Semaphore union for semaphore operations
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

int main() {
    // Read n from graph.txt
    FILE *graph_file = fopen("graph.txt", "r");
    if (!graph_file) {
        error("Error opening graph file");
    }
    int n;
    fscanf(graph_file, "%d", &n);

    // Generate keys for shared memory and semaphores using ftok
    key_t key_A = ftok("./boss.c", 'A');
    key_t key_T = ftok("./boss.c", 'T');
    key_t key_idx = ftok("./boss.c", 'i');
    key_t key_mtx = ftok("./boss.c", 'm');
    key_t key_sync = ftok("./boss.c", 's');
    key_t key_ntfy = ftok("./boss.c", 'n');

    if (key_A == -1 || key_T == -1 || key_idx == -1 || key_mtx == -1 || key_sync == -1 || key_ntfy == -1) {
        error("ftok failed");
    }

    // Create shared memory for adjacency matrix A
    int shmid_A = shmget(key_A, n * n * sizeof(int), IPC_CREAT | 0666);
    if (shmid_A == -1) {
        error("shmget failed");
    }
    int *A = (int *)shmat(shmid_A, NULL, 0);
    if (A == (int *)-1) {
        error("shmat failed");
    }

    // Initialize adjacency matrix A
    for (int i = 0; i < n * n; i++) {
        fscanf(graph_file, "%d", &A[i]);
    }
    fclose(graph_file);

    // Create shared memory for topological listing T
    int shmid_T = shmget(key_T, n * sizeof(int), IPC_CREAT | 0666);
    if (shmid_T == -1) {
        error("shmget failed");
    }
    int *T = (int *)shmat(shmid_T, NULL, 0);
    if (T == (int *)-1) {
        error("shmat failed");
    }

    // Create shared memory for index idx
    int shmid_idx = shmget(key_idx, sizeof(int), IPC_CREAT | 0666);
    if (shmid_idx == -1) {
        error("shmget failed");
    }
    int *idx = (int *)shmat(shmid_idx, NULL, 0);
    if (idx == (int *)-1) {
        error("shmat failed");
    }

    // Initialize idx
    *idx = 0;

    // Create semaphore for mutual exclusion (mtx)
    int semid_mtx = semget(key_mtx, 1, IPC_CREAT | 0666);
    if (semid_mtx == -1) {
        error("semget failed");
    }

    // Initialize semaphore mtx
    union semun sem_mtx_init;
    sem_mtx_init.val = 1;
    if (semctl(semid_mtx, 0, SETVAL, sem_mtx_init) == -1) {
        error("semctl failed");
    }

    // Create semaphore set for worker synchronization (sync)
    int semid_sync = semget(key_sync, n, IPC_CREAT | 0666);
    if (semid_sync == -1) {
        error("semget failed");
    }

    // Initialize semaphore set sync
    union semun sem_sync_init;
    sem_sync_init.val = 0;
    for (int i = 0; i < n; i++) {
        if (semctl(semid_sync, i, SETVAL, sem_sync_init) == -1) {
            error("semctl failed");
        }
    }

    // Create semaphore for notification (ntfy)
    int semid_ntfy = semget(key_ntfy, 1, IPC_CREAT | 0666);
    if (semid_ntfy == -1) {
        error("semget failed");
    }

    // Initialize semaphore ntfy
    union semun sem_ntfy_init;
    sem_ntfy_init.val = 0;
    if (semctl(semid_ntfy, 0, SETVAL, sem_ntfy_init) == -1) {
        error("semctl failed");
    }

    printf("+++ Boss: Setup done...\n");

    // Wait for notifications from all workers
    for (int i = 0; i < n; i++) {
        struct sembuf sembuf_wait;
        sembuf_wait.sem_num = 0;
        sembuf_wait.sem_op = -1;
        sembuf_wait.sem_flg = 0;
        if (semop(semid_ntfy, &sembuf_wait, 1) == -1) {
            error("semop failed");
        }
        // wait(1);
    }

    // Print topological listing T
    printf("+++ Topological Listing T:\n");
    for (int i = 0; i < n; i++) {
        printf("%d  ", T[i]);
    }

    // validation of topological order

    int* visited = (int*)malloc(n * sizeof(int)); // Keeps track of visited columns
    
    for (int i = 0; i < n; i++) {
        visited[i] = 0; // Mark all columns as not visited
    }
    for (int i = 0; i < n; i++) {
        visited[T[i]] = 1; // Mark the current column as visited
        for (int row = 0; row < n; row++) {
            if (A[row * n + T[i]] == 1) { // Check if there is an edge from row to T[i]
                if (!visited[row]) {
                    free(visited);
                    printf("+++Boss: Invalid work\n");
                    return 0; // Dependency not satisfied, invalid topological order
                }
            }
        }
    }
    
    free(visited);
    // return 1; 




    printf("\n+++ Boss: Well done,my team...\n");


    // Remove shared memory segments
    shmctl(shmid_A, IPC_RMID, NULL);
    shmctl(shmid_T, IPC_RMID, NULL);
    shmctl(shmid_idx, IPC_RMID, NULL);

    // Remove semaphores
    semctl(semid_mtx, 0, IPC_RMID);
    semctl(semid_sync, 0, IPC_RMID);
    semctl(semid_ntfy, 0, IPC_RMID);

    return 0;
}
