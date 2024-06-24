#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <utime.h>

#define buff_size 4096
#define MAXI 1024

void Notify_Change(char ch, const char *p) 
{
    printf("[%c] %s\n", ch, p);
}
void synchronize_timestamps_permissions(const char *Source, const char *directory_path)
{
    struct stat Source_file_stat;
    struct stat Destination_file_stat;
    if (stat(Source, &Source_file_stat) == -1) 
    {
        printf("Failed to open file: %s\n",Source);
        perror("Error in stat");
        exit(0);
    }
    if (stat(directory_path, &Destination_file_stat) == -1) 
    {
        printf("Failed to open file: %s\n",directory_path);
        perror("Error in stat");
        exit(0);
    }
    if(Destination_file_stat.st_mode != Source_file_stat.st_mode)
    {
        if (chmod(directory_path, Source_file_stat.st_mode) == -1) 
        {
            perror("Error in chmod");
            exit(0);
        }
        Notify_Change('p', directory_path);
    }
    if(Destination_file_stat.st_mtime != Source_file_stat.st_mtime)
    {
        struct utimbuf updated_timestamps;
        updated_timestamps.actime = Source_file_stat.st_atime;
        updated_timestamps.modtime = Source_file_stat.st_mtime;
        if (utime(directory_path, &updated_timestamps) == -1) 
        {
            perror("Error in utime");
            exit(0);
        }
        Notify_Change('t', directory_path);
    }

    DIR *dir;
    struct dirent *current_entry;
    char Source_item[MAXI];
    char target_item[MAXI];
    if((dir = opendir(directory_path)) == NULL) 
    {
        perror("Error in opendir");
        exit(0);
    }
    while((current_entry = readdir(dir)) != NULL) 
    {
        if (strcmp(current_entry->d_name, ".") == 0 || strcmp(current_entry->d_name, "..") == 0) 
        {
            continue;
        }
        snprintf(target_item, sizeof(target_item), "%s/%s", directory_path, current_entry->d_name);
        snprintf(Source_item, sizeof(Source_item), "%s/%s", Source, current_entry->d_name);
        if (stat(Source_item, &Source_file_stat) == -1) 
        {
            printf("Error %s\n",Source_item);
            perror("Error in stat");
            exit(0);
        }
        if (stat(target_item, &Destination_file_stat) == -1) 
        {
            printf("Error %s\n",target_item);
            perror("Error in stat");
            exit(0);
        }
        if (current_entry->d_type == DT_DIR) 
        {
            synchronize_timestamps_permissions(Source_item, target_item);
            continue;
        }
        if(Destination_file_stat.st_mode != Source_file_stat.st_mode)
        {
            if (chmod(target_item, Source_file_stat.st_mode) == -1) 
            {
                perror("chmod");
                exit(0);
            }
            Notify_Change('p', target_item);
        }
        if(Destination_file_stat.st_mtime != Source_file_stat.st_mtime)
        {
            struct utimbuf updated_timestamps;
            updated_timestamps.actime = Source_file_stat.st_atime;
            updated_timestamps.modtime = Source_file_stat.st_mtime;
            if (utime(target_item, &updated_timestamps) == -1) {
                perror("utime");
                exit(0);
            }
            Notify_Change('t', target_item);
        }
    }
}
void File_Copy(const char *source_path, const char *destination_path) 
{
    //File descriptor for source and in read mode
    FILE *Source_File;
    if ((Source_File = fopen(source_path, "r")) == NULL) 
    {
        printf("Failed to open file: %s\n", source_path);
        perror("fopen function error");
        exit(0);
    }
    //File descriptor for the destination and in write mode
    FILE  *Destination_File;
    if ((Destination_File = fopen(destination_path, "w")) == NULL) 
    {
        printf("Failed to open file: %s\n", destination_path);
        perror("fopen function error");
        exit(0);
    }

    //Copying content from source file to destination file
    char buff[buff_size];
    size_t numBytesRead;
    while ((numBytesRead = fread(buff, 1, buff_size, Source_File)) > 0) 
    {
        if (fwrite(buff, 1, numBytesRead, Destination_File) != numBytesRead) 
        {
            perror("fwrite function error");
            exit(0);
        }
    }
    fclose(Source_File);
    fclose(Destination_File);
}
void synchronize_files(const char *source_path, const char *destination_path, int flg) 
{
    struct stat Source_file_stat, Destination_file_stat;
    if (stat(source_path, &Source_file_stat) == -1) 
    {
        if (errno == ENOENT) 
        {
            remove(destination_path);
            Notify_Change('-', destination_path);
            return;
        } 
        else 
        {
            printf("Failed to open file: %s\n", source_path);
            perror("Error in stat");
            exit(0);
        }
    }
    if(flg)
    {
        if(S_ISDIR(Source_file_stat.st_mode))
        {
            remove(destination_path);
            Notify_Change('-',destination_path);
        }
        return;
    }
    int x = 10;
    for(int i=0;i<x;i++)
    {

    }
    if (stat(destination_path, &Destination_file_stat) == -1) 
    {
        if (errno == ENOENT) 
        {
            File_Copy(source_path, destination_path);
            Notify_Change('+', destination_path);
            return;
        } 
        else 
        {
            printf("Failed to open file: %s\n", destination_path);
            perror("Error in stat");
            exit(0);
        }
    }
    if (S_ISREG(Source_file_stat.st_mode)) 
    {
        if (Source_file_stat.st_size != Destination_file_stat.st_size || Source_file_stat.st_mtime != Destination_file_stat.st_mtime) 
        {
            File_Copy(source_path, destination_path);
            Notify_Change('o', destination_path);
        }
    } 
    else if (S_ISDIR(Source_file_stat.st_mode)) 
    {
        remove(destination_path); 
        Notify_Change('-', destination_path);
    } 
    else 
    {
        fprintf(stderr, "Error: Unexpected file type encountered while processing %s\n", source_path);
        exit(0);
    }
}

void synchronize_contents(const char *source_directory, const char *destination_directory, int flg) 
{
    DIR *dir;
    char Source_item[MAXI];
    char target_item[MAXI];
    struct dirent *current_entry;
    if(access(destination_directory, F_OK) == -1) 
    {
        if(mkdir(destination_directory, 0777) == -1) 
        {
            perror("Error in mkdir");
            exit(0);
        }
        Notify_Change('+', destination_directory);
    }
    else 
    {
        if((dir = opendir(source_directory)) == NULL) 
        {
            if (access(source_directory, F_OK) == -1) 
            {
                if (rmdir(destination_directory) == -1) 
                {
                    perror("Error in rmdir");
                    exit(0);
                }
                Notify_Change('-', destination_directory);
                return;
            }
            else 
            {
                printf("Failed to open file: %s\n", source_directory);
                perror("Error in opendir");
                exit(0);
            }
        }

        if((dir = opendir(destination_directory)) == NULL) 
        {
            snprintf(target_item, sizeof(target_item), "%s", destination_directory);
            snprintf(Source_item, sizeof(Source_item), "%s", source_directory);
            synchronize_files(Source_item, target_item,1);
        }
        
        while((dir != NULL) && (current_entry = readdir(dir)) != NULL) 
        {
            if (strcmp(current_entry->d_name, ".") == 0 || strcmp(current_entry->d_name, "..") == 0) 
            {
                continue;
            }
            snprintf(target_item, sizeof(target_item), "%s/%s", destination_directory, current_entry->d_name);
            snprintf(Source_item, sizeof(Source_item), "%s/%s", source_directory, current_entry->d_name);
            if (current_entry->d_type == DT_DIR) 
            {
                synchronize_contents(Source_item, target_item,1);
                continue;
            }
            synchronize_files(Source_item, target_item,1);
        }
    }
    int y = 15;
    while(y)
    {
        y--;
    }
    if(flg) 
    {
        return;
    }
    if ((dir = opendir(source_directory)) == NULL) 
    {
        if (access(source_directory, F_OK) == -1) 
        {
            if (rmdir(destination_directory) == -1) 
            {
                perror("Error in rmdir");
                exit(0);
            }
            Notify_Change('-', destination_directory);
            return;
        }
        else 
        {
            printf("Failed to open file: %s\n", source_directory);
            perror("Error in opendir");
            exit(0);
        }
    }

    while ((current_entry = readdir(dir)) != NULL) 
    {
        if (strcmp(current_entry->d_name, ".") == 0 || strcmp(current_entry->d_name, "..") == 0) 
        {
            continue;
        }

        snprintf(Source_item, sizeof(Source_item), "%s/%s", source_directory, current_entry->d_name);
        snprintf(target_item, sizeof(target_item), "%s/%s", destination_directory, current_entry->d_name);
        if (current_entry->d_type == DT_DIR) 
        {
            synchronize_contents(Source_item, target_item,0);
            continue;
        }
        else
        {
            synchronize_files(Source_item, target_item,0);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) 
{
    if (argc != 3) 
    {
        fprintf(stderr, "Error: Invalid number of arguments. Usage: %s <source_dir> <destination_dir>\n", argv[0]);
        exit(0);
    }

    const char *source_directory = argv[1];
    const char *destination_directory = argv[2];

    synchronize_contents(source_directory, destination_directory,0);
    synchronize_timestamps_permissions(source_directory, destination_directory);

    return 0;
}