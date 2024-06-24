#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>

// #define VERBOSE // Comment out to disable verbose output
// #define SLEEP // Comment out to disable sleep between setting M[0] and M[1]


int main(int argc, char *argv[]) {
    int n,t;
    printf("n=");
    scanf("%d",&n);
    printf("t=");
    scanf("%d",&t);

    // Producer process
     int produced[n + 1];
    int total_sum[n + 1];

    // Initialize all elements of produced and total_sum to 0
    for (int i = 0; i <= n; i++) {
        produced[i] = 0;
        total_sum[i] = 0;
    }

    if (n <= 0 || t <= 0) {
        printf("Invalid input\n");
        return 1;
    }

    printf("producer is alive\n");
    int shmid;
    key_t key = IPC_PRIVATE;
    int *M;

    // Create shared memory
    if ((shmid = shmget(key, 2 * sizeof(int), IPC_CREAT | 0777)) < 0) {
        perror("shmget");
        return 1;
    }

    // Attach to the shared memory
    if ((M = shmat(shmid, NULL, 0)) == (int *)-1) {
        perror("shmat");
        return 1;
    }

    M[0] = 0; // Initialize M[0]

    pid_t pid;
    srand(time(NULL));

    // Create consumer processes
    for (int i = 1; i <= n; i++) {
        if ((pid = fork()) < 0) {
            perror("fork");
            return 1;
        } else if (pid == 0) {
            // Consumer process

            printf("\t\t\t Consumer %d is alive\n",i);
            int items = 0;
            int sum = 0;
            while (1) {
                // Wait until M[0] becomes c or -1
                while (M[0] != i && M[0] != -1) {
                    // Busy wait
                }
                if (M[0] != -1) {

                    // Read M[1] as the next item meant for the consumer
                int item = M[1];
                M[0] = 0; // Mark as consumed
                items++;
                sum += item;
                 #ifdef VERBOSE
                    printf("\t\t\t consumer %d reads %d\n", i, item);
                #endif
                continue;

                    }
                 printf("\t\t\t Consumer %d has read %d items : checksum=%d \n",i,items,sum);

                    exit(0);
                
                
            }
        }
    }



    for (int i = 1; i <= t; i++) {
        int item = rand() % 900 + 100; // Generate random 3-digit item
        int c = rand() % n + 1; // Random consumer
        while (M[0] != 0) {
            // Busy wait
        }
        M[0] = c;

        #ifdef SLEEP
        usleep(rand() % 10 + 1); // Sleep for 1-10 microseconds
        #endif
        M[1] = item;

        #ifdef VERBOSE
        printf("producer Produced item %d for Consumer %d\n", item, c);
        #endif

        produced[c]++;
        total_sum[c] += item;
    }

    while (M[0] != 0) {
        // Busy wait
    }
    M[0] = -1; // Mark end of production

    // Wait for all consumers to terminate
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    
    // Print final statistics
    printf("producer has produced %d items\n",t);
    for (int i = 1; i <= n; i++) {
        printf("%d items for consumer %d : checksum=%d\n", produced[i],i,total_sum[i]);
    }

    // Detach from shared memory
    shmdt(M);

    // Remove shared memory
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
