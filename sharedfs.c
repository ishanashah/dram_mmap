#define _GNU_SOURCE
#include <stdio.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

typedef enum {
    MMAP,
    MUNMAP,
} request_t;

#define MAX_FILES 1024

typedef struct {
    char path[PATH_MAX];
    int memfd;
    int refcount;
} Shared_file;

Shared_file file_list[MAX_FILES];
int num_files = 0;

int search_path(char* path) {
    for (int i = 0; i < num_files; i++) {
        if(strcmp(path, file_list[i].path) == 0) {
            return i; 
        }
    }
    return -1;
}

int search_memfd(int memfd) {
    for (int i = 0; i < num_files; i++) {
        if (memfd == file_list[i].memfd) {
            return i;
        }
    }
    return -1;
}

void swap_file_list(int lhs, int rhs) {
    Shared_file temp = file_list[lhs];
    file_list[lhs] = file_list[rhs];
    file_list[rhs] = temp;
}

void copy(int fd1, int fd2) {
    char buffer[4096];
    lseek(fd1, 0, SEEK_SET);
    lseek(fd2, 0, SEEK_SET);
    int length = 0;
    int bytes = read(fd1, buffer, 4096);
    while (bytes > 0) {
        length += bytes;
        write(fd2, buffer, bytes);
        bytes = read(fd1, buffer, 4096);
    }
    ftruncate(fd2, length);
}

int handle_request(request_t type, char* filename, int memfd) {
    if (type == MMAP) {
        int index = search_path(filename);
        if (index < 0) {
            memfd = memfd_create("assise", 0);
            int fd = open(filename, O_RDONLY);
            copy(fd, memfd);
            close(fd);

            strcpy(file_list[num_files].path, filename);
            file_list[num_files].memfd = memfd;
            file_list[num_files].refcount = 1;
            num_files++;

            return memfd;

        } else {
            file_list[index].refcount++;
            return file_list[index].memfd;
        }
    } else if (type == MUNMAP) {
        int index = search_memfd(memfd);
        if (index < 0) {
            return -1;
        }
        file_list[index].refcount--;
        if (file_list[index].refcount == 0) {
            swap_file_list(index, num_files);
            int fd = open(file_list[num_files].path, O_WRONLY);
            copy(memfd, fd);
            close(fd);
            close(file_list[num_files].memfd);
            num_files--;
        }
        return 0;
    }
    return -1;
}


int main() {
    // printf() displays the string inside quotation
    printf("Hello, World!\n");
    while(1) {
        request_t type;
        char filename[PATH_MAX];
        int fd;

        //CODE TO RECIEVE REQUEST (type, fd)

        int out = handle_request(type, filename, fd);

        //CODE TO SEND RESPONSE (out)

    }
    return 0;
}
