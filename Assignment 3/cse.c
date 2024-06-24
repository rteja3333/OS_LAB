#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>


// Global variables to store child PIDs
pid_t child_pid[2];

// Signal handler function to catch termination signal
void termination_handler(int signum) {
    // Terminate both child processes
    kill(child_pid[0], SIGTERM);
    kill(child_pid[1], SIGTERM);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("+++ CSE in supervisor mode: Started\n");
        int pipe_fd[2];  // File descriptors for the first pipe
        int pipe_fd1[2]; // File descriptors for the second pipe
        

        if (pipe(pipe_fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        if (pipe(pipe_fd1) == -1) {
            perror("pipe1");
            exit(EXIT_FAILURE);
        }
                printf("+++ CSE in supervisor mode: pfd = [%d %d]\n",pipe_fd[0],pipe_fd[1]);
	int inpdup = dup(0);
        int outdup = dup(1);
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        char pipe_write_str[20], pipe_read_str[20];
        char pipe_write_str1[20], pipe_read_str1[20];

        sprintf(pipe_read_str, "%d", pipe_fd[0]);
        sprintf(pipe_write_str, "%d", pipe_fd[1]);

        sprintf(pipe_read_str1, "%d", pipe_fd1[0]);
        sprintf(pipe_write_str1, "%d", pipe_fd1[1]);

        if (pid == 0) {
            // This is the first child
            printf("+++ CSE in supervisor mode: Forking first child in command-input mode\n");
            execlp("xterm", "xterm", "-T", "First child", "-e", "./cse", "1", pipe_read_str, pipe_write_str, pipe_read_str1, pipe_write_str1, (char *)NULL);
            perror("execlp"); // If execlp fails
            exit(EXIT_FAILURE);
        } else {
            child_pid[0] = pid; // Store the first child PID
            // This is the parent
            pid_t pid1 = fork();

            if (pid1 == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid1 == 0) {
                // This is the second child
                printf("+++ CSE in supervisor mode: Forking second child in execute mode\n");
                execlp("xterm", "xterm", "-T", "Second child", "-e", "./cse", "2", pipe_read_str, pipe_write_str, pipe_read_str1, pipe_write_str1, (char *)NULL);
                perror("execlp"); // If execlp fails
                exit(EXIT_FAILURE);
            } else {
                child_pid[1] = pid1; // Store the second child PID
                // This is still the parent
                // Set up the signal handler for termination
                signal(SIGTERM, termination_handler);

                // Wait for any child process to terminate
                int status;
                pid_t terminated_child = wait(&status);

                // Print information about the terminated child
                if (terminated_child == child_pid[0]) {
                    printf("First child terminated with status %d\n", WEXITSTATUS(status));
                } else if (terminated_child == child_pid[1]) {
                    printf("Second child terminated with status %d\n", WEXITSTATUS(status));
                }

                // Terminate the other child if not already terminated
                kill(child_pid[0], SIGTERM);
                kill(child_pid[1], SIGTERM);

                // Wait for both child processes to complete
                waitpid(child_pid[0], NULL, 0);
                waitpid(child_pid[1], NULL, 0);
            }
        }
    }


    else if(atoi(argv[1])==1){
        int pipe_write_fd = atoi(argv[3]);
        int pipe_read_fd = atoi(argv[2]);

        int pipe_write_fd1 = atoi(argv[5]);
        int pipe_read_fd1 = atoi(argv[4]);
        // printf("%d,%d\n",pipe_read_fd,pipe_write_fd);
        //         printf("%d,%d\n",pipe_read_fd1,pipe_write_fd1);

        int a;
        // printf("command:");
        // scanf("%d",&a);
        int i=0;
        int ex=0;
        while (1) {
            if(ex)break;
            char command[100]; // Adjust the size as needed

            if(i==0){
                // close(0);
                // close(1);
                // dup2(3, 0);   // restoring stdin
                // dup2(pipe_write_fd,1);

            fprintf(stderr, "Enter command: "); // Print prompt using stderr
            // sleep(5);
            // fgets(command, sizeof(command), stdin);
          fgets(command, sizeof(command), stdin);
          
    
            // Remove the trailing newline character, if any
            size_t len = strlen(command);

            if (len > 0 && command[len - 1] == '\n') {
                // fprintf(stderr, "oo upd"); // Print prompt using stderr

                command[len - 1] = '\0';
            }

            if (strcmp(command, "swaprole")) {
                fprintf(stderr, "sending command %s\n", command); // Print prompt using stderr
                                 write(pipe_write_fd, command, len); // Send only the actual length, excluding the null terminator

                 if (strcmp(command, "exit") == 0) {
    fprintf(stderr, "Exiting...\n");
        system("pkill -f First child");
    // return 0;
    exit(0);
    // Use the process ID (PID) of the terminal process
    pid_t terminal_pid = getpid(); // Replace with the actual PID of the terminal

    // Send the TERM signal to the terminal process to request termination
    kill(terminal_pid, SIGTERM);

    // Alternatively, you can try using system-specific commands like the following:
    // system("pkill -f your_terminal_emulator_name");

    // Break out of the loop after sending the exit command
    break;
}
                // Send the command to the write end of the pipe
                write(pipe_write_fd, command, len); // Send only the actual length, excluding the null terminator
            } else {
                // fprintf(stderr, "swappp\n"); // Print prompt using stderr
                write(pipe_write_fd, command, len); // Send only the actual length, excluding the null terminator

                i = 1;
            }
            // close(0);
            // close(1);
            }
            else{
                // fprintf(stderr, "i==1\n"); // Print prompt using stderr
                // // close(1);
                // dup2(4, 1);   // restoring stdout

                // close(0);
                // dup2(pipe_read_fd1, 0);

               
            
            pid_t gchild=fork();
            if(gchild==0){
            // Process the received command (execute or print as needed)
                fprintf(stderr,"Received command:"); 

                ssize_t bytesRead = read(pipe_read_fd1, command, sizeof(command));
                fprintf(stderr," %s\n", command); 

                 size_t len = strlen(command);

            if (len > 0 && command[len - 1] == '\n') {
                // fprintf(stderr, "oo upd"); // Print prompt using stderr

                command[len - 1] = '\0';
            }

               if (strcmp(command, "exit") == 0) {
    fprintf(stderr, "Exiting...\n");
        system("pkill -f First child");
    // return 0;
    exit(0);
    // Use the process ID (PID) of the terminal process
    pid_t terminal_pid = getpid(); // Replace with the actual PID of the terminal

    // Send the TERM signal to the terminal process to request termination
    kill(terminal_pid, SIGTERM);

    // Alternatively, you can try using system-specific commands like the following:
    // system("pkill -f your_terminal_emulator_name");

    // Break out of the loop after sending the exit command
    break;
}
                if (bytesRead <= 0) {
                // End of input or an error occurred
                printf("error\n");
                // sleep(5);
                break;
            }
                // scanf("%d",&a);
                if(strcmp(command,"swaprole")){
                            system(command);
    
                                    exit(1);

                }
                else{
                    i=1;
                    printf("swap !!\n");
                    exit(0);
                }
            }
            else{
                int status;
                i = waitpid(gchild, &status, 0);
                printf("%d\n",status);
                i=status;

            }
            // close(1);
            // close(0);
            }
        }
    // if(ex)  ;


    }

    else if(atoi(argv[1])==2){
         int pipe_write_fd = atoi(argv[3]);
        int pipe_read_fd = atoi(argv[2]);

        int pipe_write_fd1 = atoi(argv[5]);
        int pipe_read_fd1 = atoi(argv[4]);
        int a;
        // printf("%d,%d\n",pipe_read_fd,pipe_write_fd);
        //         printf("%d,%d\n",pipe_read_fd1,pipe_write_fd1);

        // printf("execute:");
        // scanf("%d",&a);
        
        int i=0;
        int ex=0;
        while (1) {
                        if(ex)break;

            char command[100]; // Adjust the size as needed
            

            if(i==0){ 
                    // printf("11\n");
                    // close(0);
                    // close(1);
                    // dup2(pipe_read_fd, 0);

                    // dup2(4, 1);   // restoring stdout

                    // Read from the read end of the pipe

            
            pid_t gchild=fork();
            if(gchild==0){
            // Process the received command (execute or print as needed)
                printf("Received command: ");

                ssize_t bytesRead = read(pipe_read_fd, command, sizeof(command));
                printf("%s\n",command);
                 if (strcmp(command, "exit") == 0) {
    fprintf(stderr, "Exiting...\n");
    exit(0);
    fprintf(stderr, "Exiting...\n");

    // Use the process ID (PID) of the terminal process
    pid_t terminal_pid = getpid(); // Replace with the actual PID of the terminal

    // Send the TERM signal to the terminal process to request termination
    kill(terminal_pid, SIGTERM);

    // Alternatively, you can try using system-specific commands like the following:
    // system("pkill -f your_terminal_emulator_name");

    // Break out of the loop after sending the exit command
    break;
}
     
                if (bytesRead <= 0) {
                // End of input or an error occurred
                printf("error\n");
                // sleep(5);
                break;
            }
     if (strcmp(command, "exit") == 0) {
    fprintf(stderr, "Exiting...\n");
    exit(0);
    // Use the process ID (PID) of the terminal process
    pid_t terminal_pid = getpid(); // Replace with the actual PID of the terminal

    // Send the TERM signal to the terminal process to request termination
    kill(terminal_pid, SIGTERM);

    // Alternatively, you can try using system-specific commands like the following:
    // system("pkill -f your_terminal_emulator_name");

    // Break out of the loop after sending the exit command
    break;
}
                // scanf("%d",&a);
                if(strcmp(command,"swaprole")){
                            system(command);
    
                                    exit(0);

                }
                else{
                    i=1;
                    // printf("swap !!\n");
                    exit(1);
                }
               // Print using stdout
            }
            else{
                int status;
                i = waitpid(gchild, &status, 0);
                // printf("%d\n",status);
                i=status;

            }
            // close(0);
            // close(1);
            // sleep(5);
            // Execute the received command in the terminal
            // system(command);
        }
        else{
            // close(1);
            //     dup2(3, 0);   // restoring stdin
            //     dup2(pipe_write_fd1,1);

            fprintf(stderr, "Enter command: "); // Print prompt using stderr
            // sleep(5);
            // fgets(command, sizeof(command), stdin);
            fgets(command, sizeof(command),stdin);
            // fprintf(stderr,"command sending:%s\n",command);
             
            // Remove the trailing newline character, if any
            size_t len = strlen(command);
            if (len > 0 && command[len - 1] == '\n') {
                command[len - 1] = '\0';
            }
            if(strcmp(command,"swaprole")){

            // Send the command to the write end of the pipe
                fprintf(stderr,"command sending:%s\n",command);
                                write(pipe_write_fd1, command, strlen(command) + 1);

                if (strcmp(command, "exit") == 0) {
                exit(0);
                fprintf(stderr, "Exiting...\n");
                ex=1;
        break;
    }

                write(pipe_write_fd1, command, strlen(command) + 1);
            }
            else{
                 write(pipe_write_fd1, command, len); // Send only the actual length, excluding the null terminator

                i=0;
            }
            // close(0);
            // close(1);

        }


    }
    // if(ex) break;

    }


    printf("+++ CSE in supervisor mode: First child terminated\n+++ CSE in supervisor mode: Second child terminated");
    return 0;
}
