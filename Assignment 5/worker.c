    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/ipc.h>
    #include <sys/shm.h>
    #include <sys/sem.h>
    #include <sys/types.h>

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

    int main(int argc, char *argv[]) {
        if (argc != 3) {
            fprintf(stderr, "Usage: %s n worker_id\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        int n = atoi(argv[1]);
        int worker_id = atoi(argv[2]);

        sleep(1);
        // printf("workerid=%d\n",worker_id);
        // Get keys for shared memory and semaphore sets
        key_t key_A = ftok("./boss.c", 'A');
        key_t key_T = ftok("./boss.c", 'T');
        key_t key_idx = ftok("./boss.c", 'i');
        key_t key_mtx = ftok("./boss.c", 'm');
        key_t key_sync = ftok("./boss.c", 's');
        key_t key_ntfy = ftok("./boss.c", 'n');

        if (key_A == -1 || key_T == -1 || key_idx == -1 || key_mtx == -1 || key_sync == -1 || key_ntfy == -1) {
            error("ftok failed");
        }

        // Attach to shared memory segments
        int shmid_A = shmget(key_A, n * n * sizeof(int), 0666);
        int shmid_T = shmget(key_T, n * sizeof(int), 0666);
        int shmid_idx = shmget(key_idx, sizeof(int), 0666);

        if (shmid_A == -1 || shmid_T == -1 || shmid_idx == -1) {
            error("shmget failed");
        }

        int *A = (int *)shmat(shmid_A, NULL, 0);
        int *T = (int *)shmat(shmid_T, NULL, 0);
        int *idx = (int *)shmat(shmid_idx, NULL, 0);

        if (A == (int *)-1 || T == (int *)-1 || idx == (int *)-1) {
            error("shmat failed");
        }

        // Attach to semaphore for mutual exclusion (mtx)
        int semid_mtx = semget(key_mtx, 1, 0666);
        if (semid_mtx == -1) {
            error("semget failed");
        }

        // Attach to semaphore set for worker synchronization (sync)
        int semid_sync = semget(key_sync, n, 0666);
        if (semid_sync == -1) {
            error("semget failed");
        }

        // Attach to semaphore for notification (ntfy)
        int semid_ntfy = semget(key_ntfy, 1, 0666);
        if (semid_ntfy == -1) {
            error("semget failed");
        }

        // Wait for sync signals from incoming links (j, i)
        for (int j = 0; j < n; j++) {
            if (A[j * n + worker_id]) {
                struct sembuf sembuf_sync;
                sembuf_sync.sem_num = j;
                sembuf_sync.sem_op = -1;
                sembuf_sync.sem_flg = 0;
                semop(semid_sync, &sembuf_sync, 1);
                // printf("///signal recieved from %d to %d\n",j,worker_id);
            }
        }

        // Critical section: Write worker_id to T and increment idx
        struct sembuf sembuf_mtx;
        sembuf_mtx.sem_num = 0;
        sembuf_mtx.sem_op = -1; // Lock
        sembuf_mtx.sem_flg = 0;
        semop(semid_mtx, &sembuf_mtx, 1);
        // printf("\nworker %d completed work",worker_id);
        T[*idx] = worker_id;
        (*idx)++;

        sembuf_mtx.sem_op = 1; // Unlock
        semop(semid_mtx, &sembuf_mtx, 1);

        // Send sync signals to outgoing links (i, k)
        for (int k = 0; k < n; k++) {
            if (A[worker_id * n + k]) {
                struct sembuf sembuf_sync;
                sembuf_sync.sem_num = worker_id;
                sembuf_sync.sem_op = 1;
                sembuf_sync.sem_flg = 0;
                semop(semid_sync, &sembuf_sync, 1);
                // printf("\t\t\tsending signal from %d to process=%d\n",worker_id,k);
            }
        }

        // Notify B using the semaphore ntfy
        struct sembuf sembuf_ntfy;
        sembuf_ntfy.sem_num = 0;
        sembuf_ntfy.sem_op = 1;
        sembuf_ntfy.sem_flg = 0;
        semop(semid_ntfy, &sembuf_ntfy, 1);

        // Detach from shared memory and semaphores
        shmdt(A);
        shmdt(T);
        shmdt(idx);

        return 0;
    }
