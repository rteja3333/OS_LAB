#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/select.h>

#define MAX_LENGTH 100

typedef struct {
    int jobNumber;
    int processId;
    int processGroupId;
    char status[MAX_LENGTH];
    char jobName[MAX_LENGTH];
} JobInfo;

int globalProcessId = -2;
volatile int controlFlag = 0;

typedef struct {
    int jobCount;
    JobInfo jobs[11];
} JobTable;

JobTable jobTable;

void interruptHandler() {
    kill(globalProcessId, SIGINT);
    printf("\n");
}

void suspendHandler() {
    kill(globalProcessId, SIGTSTP);
    printf("\n");
}

void noOperation() {
    controlFlag = 1;
}

void printHelpMessage() {
    printf("\tCommand : Action\n");
    printf("\th\t: Print this help message\n");
    printf("\tc\t: Continue a suspended job\n");
    printf("\tk\t: Kill a suspended job\n");
    printf("\tp\t: Print the job table\n");
    printf("\tq\t: Quit\n");
    printf("\tr\t: Run a new job\n");
}

void continueJob() {
    int suspendedJobIndices[11];
    int suspendedJobCount = 0;

    for (int i = 0; i <= jobTable.jobCount; i++) {
        if (strcmp(jobTable.jobs[i].status, "SUSPENDED") == 0) {
            suspendedJobIndices[i] = 1;
            suspendedJobCount++;
        } else {
            suspendedJobIndices[i] = -1;
        }
    }

    if (suspendedJobCount == 0) {
        printf("No suspended jobs to continue\n");
    } else {
        printf("Suspended jobs : ");

        for (int i = 0; i <= jobTable.jobCount; i++) {
            if (suspendedJobIndices[i] == 1) printf("%d  ", i);
        }

        printf("\t(Pick one) : ");
        int selectedIndex;
        scanf("%d", &selectedIndex);

        if (selectedIndex < 0 || selectedIndex > jobTable.jobCount || suspendedJobIndices[selectedIndex] != 1) {
            printf("Choose the correct suspended job\n");
            return;
        }

        globalProcessId = jobTable.jobs[selectedIndex].processId;
        signal(SIGINT, interruptHandler);
        signal(SIGTSTP, suspendHandler);
        kill(globalProcessId, SIGCONT);

        int status;

        int terminatedProcessId = waitpid(globalProcessId, &status, WUNTRACED);

        if (WIFEXITED(status)) {
            strcpy(jobTable.jobs[selectedIndex].status, "FINISHED");
        } else if (WIFSIGNALED(status)) {
            strcpy(jobTable.jobs[selectedIndex].status, "TERMINATED");
        } else if (WIFSTOPPED(status)) {
            strcpy(jobTable.jobs[selectedIndex].status, "SUSPENDED");
        }
    }
}

void killJob() {
    int suspendedJobIndices[11];
    int suspendedJobCount = 0;

    for (int i = 0; i <= jobTable.jobCount; i++) {
        if (strcmp(jobTable.jobs[i].status, "SUSPENDED") == 0) {
            suspendedJobIndices[i] = 1;
            suspendedJobCount++;
        } else {
            suspendedJobIndices[i] = -1;
        }
    }

    if (suspendedJobCount == 0) {
        printf("No suspended jobs to kill\n");
    } else {
        printf("Suspended jobs : ");

        for (int i = 0; i <= jobTable.jobCount; i++) {
            if (suspendedJobIndices[i] == 1) printf("%d  ", i);
        }

        printf("\t(Pick one) : ");
        int selectedIndex;
        scanf("%d", &selectedIndex);

        if (selectedIndex < 0 || selectedIndex > jobTable.jobCount || suspendedJobIndices[selectedIndex] != 1) {
            printf("Choose the correct suspended job\n");
            return;
        }

        strcpy(jobTable.jobs[selectedIndex].status, "KILLED");
    }
}

void printJobTable() {
    printf("\nNO \t\t PID \t\t PGID \t\t STATUS \t JOB NAME\n");
    printf("%d \t\t %d \t\t %d \t\t %s \t\t %s\n",
           jobTable.jobs[0].jobNumber,
           jobTable.jobs[0].processId,
           jobTable.jobs[0].processGroupId,
           jobTable.jobs[0].status,
           jobTable.jobs[0].jobName);

    for (int i = 1; i <= jobTable.jobCount; i++) {
        printf("%d \t\t %d \t\t %d \t\t %s \t job %s\n",
               jobTable.jobs[i].jobNumber,
               jobTable.jobs[i].processId,
               jobTable.jobs[i].processGroupId,
               jobTable.jobs[i].status,
               jobTable.jobs[i].jobName);
    }
}

void quitProgram() {
    char suspended[] = "SUSPENDED";

    for (int i = 0; i <= jobTable.jobCount; i++) {
        if (strcmp(jobTable.jobs[i].status, suspended) == 0) {
            strcpy(jobTable.jobs[i].status, "KILLED");
        }
    }
    printf("killed the remaining suspended jobs.. \nQuitting!!\n");
    exit(0);
}

void runNewJob() {
    if (jobTable.jobCount >= 10) {
        printf("Job table is full. Quitting...\n");
        exit(-1);
    }

    int newProcessId = fork();
    char randomCharacter = 'A' + rand() % 26;
    char randomCharacterString[2];
    sprintf(randomCharacterString, "%c", randomCharacter);

    if (newProcessId == 0) {
        // Set the child process group ID to its own process ID
        setpgid(getpid(), getpid());
        execl("./job", "./job", randomCharacterString, (char *)NULL);
    } else {
        signal(SIGINT, interruptHandler);
        signal(SIGTSTP, suspendHandler);

        globalProcessId = newProcessId;

        int status;
        int terminatedProcessId = waitpid(newProcessId, &status, WUNTRACED);

        if (WIFEXITED(status)) {
            jobTable.jobCount++;
            jobTable.jobs[jobTable.jobCount].jobNumber = jobTable.jobCount;
            jobTable.jobs[jobTable.jobCount].processId = newProcessId;
            jobTable.jobs[jobTable.jobCount].processGroupId = newProcessId;  // Set pgid to newProcessId
            strcpy(jobTable.jobs[jobTable

.jobCount].status, "FINISHED");
            strcpy(jobTable.jobs[jobTable.jobCount].jobName, randomCharacterString);

        } else if (WIFSIGNALED(status)) {
            jobTable.jobCount++;
            jobTable.jobs[jobTable.jobCount].jobNumber = jobTable.jobCount;
            jobTable.jobs[jobTable.jobCount].processId = newProcessId;
            jobTable.jobs[jobTable.jobCount].processGroupId = newProcessId;  // Set pgid to newProcessId
            strcpy(jobTable.jobs[jobTable.jobCount].status, "TERMINATED");
            strcpy(jobTable.jobs[jobTable.jobCount].jobName, randomCharacterString);

        } else if (WIFSTOPPED(status)) {
            jobTable.jobCount++;
            jobTable.jobs[jobTable.jobCount].jobNumber = jobTable.jobCount;
            jobTable.jobs[jobTable.jobCount].processId = newProcessId;
            jobTable.jobs[jobTable.jobCount].processGroupId = newProcessId;  // Set pgid to newProcessId
            strcpy(jobTable.jobs[jobTable.jobCount].status, "SUSPENDED");
            strcpy(jobTable.jobs[jobTable.jobCount].jobName, randomCharacterString);
        }
    }
}

int main() {
    srand((unsigned int)time(NULL));

    // Initialize the job table with information about "SELF"
    jobTable.jobCount = 0;
    jobTable.jobs[0].jobNumber = 0;
    jobTable.jobs[0].processId = getpid();
    jobTable.jobs[0].processGroupId = getpgid(getpid());
    strcpy(jobTable.jobs[0].status, "SELF");
    strcpy(jobTable.jobs[0].jobName, "mgr");

    printf("\n\tCommand : Action\n");
    printHelpMessage();

    while (1) {
        struct sigaction sa;
        sa.sa_handler = noOperation;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTSTP, &sa, NULL);

        controlFlag = 0;

        printf("\nmgr> ");
        char command;

        scanf(" %c", &command);

        if (controlFlag == 1) continue;

        switch(command) {
            case 'h':
                printHelpMessage();
                break;
            case 'c':
                continueJob();
                break;
            case 'k':
                killJob();
                break;
            case 'p':
                printJobTable();
                break;
            case 'q':
                quitProgram();
                break;
            case 'r':
                runNewJob();
                break;
            default:
                printf("Enter a valid command\n");
        }
    }
}
