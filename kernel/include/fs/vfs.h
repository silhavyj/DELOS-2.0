#ifndef _FS_H_
#define _FS_H_

#include <mem/paging.h>
#include <math.h>

#define FILE_NAME_LEN       16
#define EOF_CLUSTER         0xFFF
#define FREE_CLUSTER        0xFFE
#define TAKEN_CLUSTER       0xFFD
#define CLUSTER_SIZE        64
#define CLUSTER_COUNT       (min(0xFFF, (FS_SIZE / (sizeof(fat12_t) + CLUSTER_SIZE))))
#define FAT_TABLE_SIZE      (sizeof(fat12_t) * CLUSTER_COUNT)
#define CLUSTER_START_ADDR  (FS_START_ADDR + FAT_TABLE_SIZE)
#define ROOT_FOLDER_SIZE    (sizeof(uint32_t) + (root->file_count * sizeof(file_t)))
#define CLUSTER_ADDR(index) (CLUSTER_START_ADDR + ((index) * CLUSTER_SIZE))

#define ROOT_FIRST_START_CLUSTER   0
#define MAX_FILES_IN_FIRST_CLUSTER ((CLUSTER_SIZE - sizeof(uint32_t)) / sizeof(file_t))
#define MAX_FILES_IN_ONE_CLUSTER   (CLUSTER_SIZE / sizeof(file_t))

typedef struct {
    uint32_t value : 12;            // fat[i] value
} __attribute__((packed)) fat12_t;

typedef struct {
    char name[FILE_NAME_LEN];       // file name
    uint32_t start_cluster_index;   // start cluster
    uint32_t size;                  // size of the file in B
    uint8_t open : 1;               // flag if the file is open
    uint8_t system: 1;              // flag if the file is a system file
} __attribute__((packed)) file_t;

typedef struct {
    uint32_t file_count;            // number of files in the root dir
    file_t *files;                  // the files themselves
} __attribute__((packed)) folder_t;

int fs_init();
void ls();
int touch(char *filename);
int rm(char *filename);
int cat(char *filename);
int cp(char *src, char *des);
int read(char *filename, char *buffer, uint32_t offset, uint32_t len);
int write(char *filename, char *buffer, uint32_t offset, uint32_t len);
int is_file_open(char *filename);
int open_file(char *filename);
int close_file(char *filename);
uint32_t get_free_cluster_count();
uint32_t get_file_size(char *filename);
uint32_t get_memory_available();
int file_exists(char *filename);
int set_as_system_file(char *filename);
int delete_system_file(char *filename);

// just for debugging purposes
void print_FAT(uint32_t n);

#endif
