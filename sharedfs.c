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
    ino_t inode;
    int memfd;
    int refcount;
} Shared_file;

Shared_file file_list[MAX_FILES];
int num_files = 0;

int search_inode(ino_t inode) {
    for (int i = 0; i < num_files; i++) {
        if (inode == file_list[i].inode) {
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

int handle_request(request_t type, ino_t inode) {
    if (type == MMAP) {
        int index = search_inode(inode);
        if (index < 0) {
            int memfd = memfd_create("assise", 0);

            file_list[num_files].inode = inode;
            file_list[num_files].memfd = memfd;
            file_list[num_files].refcount = 1;
            num_files++;

            return memfd;

        } else {
            file_list[index].refcount++;
            return file_list[index].memfd;
        }
    } else if (type == MUNMAP) {
        int index = search_inode(inode);
        if (index < 0) {
            return -1;
        }
        file_list[index].refcount--;
        if (file_list[index].refcount == 0) {
            swap_file_list(index, num_files-1);
            close(file_list[num_files-1].memfd);
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
        ino_t inode;

        //CODE TO RECIEVE REQUEST (type, fd)

        int out = handle_request(type, inode);

        //CODE TO SEND RESPONSE (out)

    }
    return 0;
}
