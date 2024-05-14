#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define MAX_PATH_LENGTH 4096
#define MAX_FILENAME_LENGTH 256

long calculate_directory_size(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return -1;
    }

    long total_size = 0;
    struct dirent *entry;

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        printf("Unable to execute\n");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char entry_path[MAX_PATH_LENGTH];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", dir_path, entry->d_name);

        struct stat entry_stat;
        if (lstat(entry_path, &entry_stat) == -1) {
            printf("Unable to execute\n");
            continue;
        }

        if (S_ISLNK(entry_stat.st_mode)) {
            char link_target[MAX_PATH_LENGTH];
            ssize_t link_size = readlink(entry_path, link_target, sizeof(link_target) - 1);
            if (link_size != -1) {
                link_target[link_size] = '\0';
                total_size += calculate_directory_size(link_target);
            }
        } else if (S_ISDIR(entry_stat.st_mode)) {
            pid_t child_pid;
            if ((child_pid = fork()) == -1) {
                printf("Unable to execute\n");
                continue;
            }

            if (child_pid == 0) {
                close(pipe_fd[0]);
                int sub_dir_size = calculate_directory_size(entry_path);
                write(pipe_fd[1], &sub_dir_size, sizeof(sub_dir_size));
                close(pipe_fd[1]);
                exit(0);
            }
        } else if (S_ISREG(entry_stat.st_mode)) {
            total_size += entry_stat.st_size;
        }
    }

    closedir(dir);

    struct stat root_stat;
    if (lstat(dir_path, &root_stat) != -1) {
        total_size += root_stat.st_size;
    }

    close(pipe_fd[1]);
    int child_size;
    while (read(pipe_fd[0], &child_size, sizeof(child_size)) == sizeof(child_size)) {
        total_size += child_size;
    }
    close(pipe_fd[0]);

    return total_size;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    const char *root_dir = argv[1];

    long total_size = calculate_directory_size(root_dir);

    if (total_size == -1) {
        printf("Unable to execute\n");
        return 1;
    }

    printf("%ld\n", total_size);

    return 0;
}