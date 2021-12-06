#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <linux/limits.h>


#include <sys/types.h>
#include <sys/stat.h>


#define MAX_MAPPINGS 1024

typedef struct {
    void* addr;
    size_t length;
    int prot;
    int flags;
    int fd;
    off_t offset;
    ino_t inode;
    int memfd;
} Mapping;

Mapping map_list[MAX_MAPPINGS];
int map_size = 0;

/*
TODO
Send a request to the server.
Send the inode of an assise file to the server.
The server responds with a memfd if it already exists for this assie file.
Otherwise, the server allocates and returns a new empty memfd for the file.
The function returns the memfd.
*/
int request_mmap_shared(ino_t inode) {
    perror("request_mmap_shared NOT IMPLEMENTED");
    return 0;
}


/*
TODO
Send a request to the server
Send the inode of an assise file to the server.
The server cleans up any state related to the inode.
The function returns without waiting for acknowledgement.
*/
void request_munmap_shared(ino_t inode) {
    perror("request_munmap_shared NOT IMPLEMENTED");
}


int search_addr(void* addr) {
    for (int i = 0; i < map_size; i++) {
        if (((uint64_t)addr >= (uint64_t)map_list[i].addr) &&
                ((uint64_t)addr < (uint64_t)map_list[i].addr +
                map_list[i].length)) {
            return i;
        }
    }
    return -1;
}

void swap_map_list(int lhs, int rhs) {
    Mapping temp = map_list[lhs];
    map_list[lhs] = map_list[rhs];
    map_list[rhs] = temp;
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

void* mmap_assise(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    if (map_size == MAX_MAPPINGS) {
        printf("MAX NUMBER OF MAPPINGS REACHED");
        return NULL;
    }
    if (flags & MAP_SHARED) {
        //printf("SHARED MAPPING NOT IMPLEMENTED\n");
        map_list[map_size].length = length;
        map_list[map_size].prot = prot;
        map_list[map_size].flags = flags;
        map_list[map_size].fd = dup(fd);
        map_list[map_size].offset = offset;

        struct stat sb;
        fstat(fd, &sb);
        map_list[map_size].inode = sb.st_ino;

        int memfd = request_mmap_shared(sb.st_ino);
        map_list[map_size].memfd = memfd;
        fstat(memfd, &sb);
        if (sb.st_size == 0) {
            copy(fd, memfd);
        }

        void* buffer = mmap(addr, length, flags,
                MAP_SHARED, memfd, offset);
        map_list[map_size].addr = buffer;

        map_size++;

        return buffer;
    } else {
        void* buffer = mmap(addr, length, flags | PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
        lseek(fd, offset, SEEK_SET);
        read(fd, buffer, length);

        map_list[map_size].addr = buffer;
        map_list[map_size].length = length;
        map_list[map_size].prot = prot;
        map_list[map_size].flags = flags;
        map_list[map_size].fd = fd;
        map_list[map_size].offset = offset;
        map_size++;

        return buffer;
    }
    return NULL;
}

int msync_assise(void *addr, size_t length, int flags) {
    if (flags & MS_ASYNC) {
        return 0;
    }
    int index = search_addr(addr);
    if (index < 0) {
        printf("INVALID ADDRESS\n");
        return -1;
    }
    if ((map_list[index].prot & PROT_WRITE) == 0) {
        return 0;
    }
    if (map_list[index].flags & MAP_PRIVATE) {
        return 0;
    }
    lseek(map_list[index].fd, map_list[index].offset + (uint64_t)addr -
            (uint64_t)map_list[index].addr, SEEK_SET);
    write(map_list[index].fd, map_list[index].addr, map_list[index].length);
    return 0;
}

int munmap_assise(void *addr, size_t len) {
    //CURRENTLY MUNMAPS WHOLE ALLOCATION
    int index = search_addr(addr);
    if (index < 0) {
        printf("INVALID ADDRESS\n");
        return -1;
    }
    swap_map_list(index, map_size);
    if (map_list[map_size].flags & MAP_SHARED) {
        msync_assise(map_list[map_size].addr, map_list[map_size].length, MS_SYNC);
        request_munmap_shared(map_list[map_size].inode);
    }
    munmap(map_list[map_size].addr, map_list[map_size].length);
    map_size--;
    return 0;
}

int main() {
    // printf() displays the string inside quotation
    printf("Hello, World!\n");
    int fd = open("file.txt", O_RDONLY);


    //char* buffer = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    //printf("FILE CONTENTS:%s\n", buffer);
    char* buffer = mmap_assise(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    printf("BUFFER: %ld\n", (uint64_t)buffer);
    printf("FILE CONTENTS:%s\n", buffer);
    char* s = "HELLO THIS IS MOD";
    memcpy(buffer, s, 4096);
    msync_assise(buffer, 4096, MS_SYNC);
    printf("BUFFER CONTENTS:%s\n", buffer);

    close(fd);
   

    return 0;
}
