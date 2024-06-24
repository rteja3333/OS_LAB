#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_CHILDREN 5
#define BUFFER_SIZE 1024
#define NUM_ARGS 3

void read_file(FILE* file, const char* city_name, int level) {
    char buffer[BUFFER_SIZE];
    rewind(file);
    int flag=0;

    while (fgets(buffer, sizeof(buffer), file)) {
        char* token = strtok(buffer, " ");
        if (strcmp(token, city_name) != 0) {
            continue;
        }
        flag=1;
        token = strtok(NULL, " ");
        int num_children = atoi(token);
        pid_t pid = getpid();
        // Print indented city name
        printf("%*.s%s (%d)\n", level * 4, "", city_name, pid);

        if (num_children <= 0 || num_children > MAX_CHILDREN) {
            continue;
        }

        for (int i = 0; i < num_children; ++i) {
            token = strtok(NULL, " \n");
            char level_str[10];
            snprintf(level_str, sizeof(level_str), "%d", level + 1);
            pid_t child_pid = fork();
            if (child_pid == 0) {
            execl("./proctree","proctree", token, level_str, NULL);
            perror("Exec Failed!");
            exit(EXIT_FAILURE);
            } else {
               int child_exit_code;
               waitpid(child_pid, &child_exit_code, 0);
            }
            memset(token, '\0', strlen(token));
        }
    }
    if (flag==0) {
        printf("City not found\n");
    }
}

int main(int argc, char** argv) {
    srand(time(NULL));

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Run with a node name\n");
        exit(EXIT_FAILURE);
    }

    int level = 0;
    if (argc == 3) {
        level = atoi(argv[2]);

    }

    FILE* file = fopen("treeinfo.txt", "r");
    if (file == NULL) {
        perror("Could not open file.\n");
        exit(EXIT_FAILURE);
    }

    read_file(file, argv[1], level);
    fclose(file);

    return EXIT_SUCCESS;
}