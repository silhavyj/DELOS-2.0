#include <fs/vfs.h>
#include <mem/heap.h>
#include <common.h>
#include <memory.h>
#include <string.h>
#include <drivers/screen/screen.h>

static volatile fat12_t *fat = reinterpret_cast<fat12_t *>(FS_START_ADDR);
static char file_buffer[SCREEN_BUFFER_SIZE];

static void save_root_folder(folder_t *root);
static uint32_t get_cluster_count_needed(uint32_t size);
static int exists_n_free_clusters(uint32_t n);
static uint32_t get_free_cluster();
static void free_all_occupied_clusters(uint32_t start_cluster);
static folder_t *load_root_folder();
static void normalize_filename(char *filename);
static int exists_file(const char *filename);
static void free_root_dir(folder_t *root);
static void create_file(folder_t *root, const char *filename);
static void delete_file(folder_t *root, const char *filename);
static file_t *get_file(folder_t *root, char *filename);
static void create_default_files();
static void append_data(char *filename, char *buffer, uint32_t bytes);

uint32_t get_free_cluster_count() {
    uint32_t free_cluster = 0;
    uint32_t i;
    for (i = 0; i < CLUSTER_COUNT; i++)
        free_cluster += (fat[i].value == FREE_CLUSTER);
    return free_cluster;
}

static void create_default_files() {
    // a help buffer used to create default files
    char buff[256];

    // create a file that contains a brief readme kind of content
    strcpy(buff, "Welcome to a pseudo-FAT12-based file system. This file system supports basic operations like cat, ls, touch, cp, etc. It was created as a part of the semestral project of the KIV/OS module in 2021.\n\r");
    touch("README.md");
    append_data("README.md", buff, strlen(buff));

    // create a file that contains ascii characters (not all of them, just the printable ones)
    memset(buff, 0, 256);
    char c;
    for (c = '!'; c <= '~'; c++)
        append(buff, c);
    touch("ascii.txt");
    append_data("ascii.txt", buff, strlen(buff));

    // create a file that contains all prime numbers from 2 to 100
    memset(buff, 0, 256);
    strcpy(buff, "2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97");
    touch("primes.dat");
    append_data("primes.dat", buff, strlen(buff));
}

static void free_root_dir(folder_t *root) {
    // make sure the root dir has not
    // been already deallocated
    if (root == NULL)
        return;

    // if the number of files is > 0
    // deallocate the files first
    if (root->files != NULL) {
        kfree(root->files);
        root->files = NULL;
    }
    // deallocate the root dir itself
    // and set the pointer to NULL
    kfree(root);
    root = NULL;
}

static int exists_file(const char *filename) {
    folder_t *root = load_root_folder();
    // go through the files of the root dir
    // and keep comparing their names with
    // the filename given as a parameter
    uint32_t i;
    for (i = 0; i < root->file_count; i++)
        if (strcmp(root->files[i].name, filename) == 0)
            return 1;
    return 0;
}

static void normalize_filename(char *filename) {
    // if the length of the file name
    // exceeds FILE_NAME_LEN cut off the tail of it
    uint32_t len = strlen(filename);
    if (len >= FILE_NAME_LEN)
        filename[FILE_NAME_LEN - 1] = '\0';
}

static void free_all_occupied_clusters(uint32_t start_cluster) {
    uint32_t curr_cluster = start_cluster;
    uint32_t prev_cluster;

    // skip the first cluster so each file starts
    // at the same position once it has been created
    curr_cluster = fat[curr_cluster].value;

    // iterate through the FAT table and keep setting the clusters as free
    // until you either reach teh EOF_CLUSTER
    while (fat[curr_cluster].value != EOF_CLUSTER && fat[curr_cluster].value != FREE_CLUSTER) {
        prev_cluster = curr_cluster;
        curr_cluster = fat[curr_cluster].value;
        fat[prev_cluster].value = FREE_CLUSTER;
    }
    // set the EOF cluster as free as well
    fat[curr_cluster].value = FREE_CLUSTER;
}

static int exists_n_free_clusters(uint32_t n) {
    uint32_t i;
    uint32_t free_cluster = 0;

    // iterate through the clusters and keep counting
    // free clusters, if you've seen at least n free
    // clusters, return 1
    for (i = 0; i < CLUSTER_COUNT; i++) {
        free_cluster += (fat[i].value == FREE_CLUSTER);
        if (free_cluster >= n)
            return 1;
    }
    return 0;
}

static uint32_t get_cluster_count_needed(uint32_t size) {
    // calculate the number of clusters needed to
    // store an "object" of a particular size
    uint32_t clusters_needed = size / CLUSTER_SIZE;
    if (size % CLUSTER_SIZE != 0)
        clusters_needed++;
    return clusters_needed;
}

static uint32_t get_free_cluster() {
    // find the first cluster, set it as TAKEN,
    // so it will not be used again and return its index
    uint32_t i;
    for (i = 0; i < CLUSTER_COUNT; i++)
        if (fat[i].value == FREE_CLUSTER) {
            fat[i].value = TAKEN_CLUSTER;
            return i;
        }
    // we're out of free clusters
    // TODO we should probably not panic the system
    set_color(FOREGROUND_RED);
    kprintf("ERR: all clusters are full");
    _panic();
    // TODO this should be checked by all functions that call get_free_cluster()
    return TAKEN_CLUSTER;
}

int fs_init() {
    uint32_t i;

    // set all clusters as free
    for (i = 0; i < CLUSTER_COUNT; i++)
        fat[i].value = FREE_CLUSTER;

    // create a root directory that will start at cluster 0
    folder_t *root = (folder_t *)kmalloc(sizeof(folder_t));
    root->file_count = 0;
    root->files = NULL;
    fat[0].value = TAKEN_CLUSTER;

    // save the root directory at the very beginning of the clusters
    save_root_folder(root);

    // deallocate it as it is not needed anymore
    free_root_dir(root);

    // create some default files as a proof of concept
    create_default_files();
    return 0;
}

static void save_root_folder(folder_t *root) {
    // calculate the total number of clusters needed to store the root dir
    uint32_t clusters_needed = get_cluster_count_needed(ROOT_FOLDER_SIZE);

    // make sure we have enough clusters to store the root dir
    // the first cluster (0) is automatically assumed to be there,
    // therefore we actually need clusters_needed - 1. However, we also
    // need the EOF cluster, therefore +1
    if (exists_n_free_clusters(clusters_needed) == 0) {
        set_color(FOREGROUND_RED);
        kprintf("ERR: there is not enough space to store the root folder");
        _panic();
    }

    uint32_t prev_cluster;
    uint32_t curr_cluster = ROOT_FIRST_START_CLUSTER;

    // store the number of files at very beginning of the first cluster
    memcpy((void *)CLUSTER_ADDR(curr_cluster), &root->file_count, sizeof(uint32_t));

    // store as many files as you can into the first cluster as well
    // the number of files is determined as min(MAX_FILES_IN_FIRST_CLUSTER, root->file_count)
    memcpy((void *)(CLUSTER_ADDR(curr_cluster) + sizeof(uint32_t)), root->files, min(MAX_FILES_IN_FIRST_CLUSTER, root->file_count) * sizeof(file_t));

    // if there's no more files to store (we don't need more than the first cluster)
    // add the EOF cluster to the end of the chain, and we're done
    if (root->file_count <= MAX_FILES_IN_FIRST_CLUSTER) {
        prev_cluster = curr_cluster;
        curr_cluster = get_free_cluster();

        fat[prev_cluster].value = curr_cluster;
        fat[curr_cluster].value = EOF_CLUSTER;
        return;
    }

    uint32_t file_index = MAX_FILES_IN_FIRST_CLUSTER; // index of the current file to be stored
    prev_cluster = curr_cluster;
    uint32_t i;

    // the last cluster needs to be dealt with separately
    // as it's not fully taken up by files (there's some space left)
    for (i = 0; i < clusters_needed - 2; i++) {
        // create a link in the FAT table
        curr_cluster = get_free_cluster();
        fat[prev_cluster].value = curr_cluster;
        prev_cluster = curr_cluster;

        // store as many files as you can into the current cluster and move on to the next one
        memcpy((void *)CLUSTER_ADDR(curr_cluster), &root->files[file_index], MAX_FILES_IN_ONE_CLUSTER * sizeof(file_t));
        file_index += MAX_FILES_IN_ONE_CLUSTER;
    }

    // create a link in the FAT table
    curr_cluster = get_free_cluster();
    fat[prev_cluster].value = curr_cluster;

    // store the remaining files into the last cluster
    memcpy((void *)CLUSTER_ADDR(curr_cluster), &root->files[file_index], (root->file_count - file_index) * sizeof(file_t));

    // attach an EOF cluster to the end
    // of the chain within the FAT table
    prev_cluster = curr_cluster;
    curr_cluster = get_free_cluster();
    fat[prev_cluster].value = curr_cluster;
    fat[curr_cluster].value = EOF_CLUSTER;
}

static folder_t *load_root_folder() {
    uint32_t curr_cluster = ROOT_FIRST_START_CLUSTER;
    folder_t *root = (folder_t *)kmalloc(sizeof(folder_t));

    // read the number of files off of the first cluster
    // and allocate an array for them
    memcpy(&root->file_count, (void *)CLUSTER_ADDR(ROOT_FIRST_START_CLUSTER), sizeof(uint32_t));
    root->files = (file_t *)kmalloc(root->file_count * sizeof(file_t));

    // read all files store in the first cluster
    memcpy(root->files, (void *)(CLUSTER_ADDR(curr_cluster) + sizeof(uint32_t)), min(MAX_FILES_IN_FIRST_CLUSTER, root->file_count) * sizeof(file_t));

    // if the number of files (their size) doesn't exceed one cluster, we're done
    if (root->file_count <= MAX_FILES_IN_FIRST_CLUSTER) {
        // sanity check of that we've reached the end
        curr_cluster = fat[curr_cluster].value;
        if (fat[curr_cluster].value != EOF_CLUSTER) {
            set_color(FOREGROUND_RED);
            kprintf("ERR: the rood directory was not loaded properly (missing EOF)");
            _panic();
        }
        return root;
    }

    // index of the current cluster to be read
    uint32_t file_index = MAX_FILES_IN_FIRST_CLUSTER;

    // process all clusters except for the last one which
    // has to be processed separately as it is not fully occupied
    while (fat[fat[fat[curr_cluster].value].value].value != EOF_CLUSTER) {
        curr_cluster = fat[curr_cluster].value;
        memcpy(&root->files[file_index], (void *)CLUSTER_ADDR(curr_cluster), MAX_FILES_IN_ONE_CLUSTER * sizeof(file_t));
        file_index += MAX_FILES_IN_ONE_CLUSTER;
    }

    // load the remaining files off of the last cluster
    curr_cluster = fat[curr_cluster].value;
    memcpy(&root->files[file_index], (void *)CLUSTER_ADDR(curr_cluster), (root->file_count - file_index) * sizeof(file_t));

    // make sure the that's the end of the chain in the FAT table
    // (the value of the next cluster is indeed EOF_CLUSTER)
    // just a sanity check
    curr_cluster = fat[curr_cluster].value;
    if (fat[curr_cluster].value != EOF_CLUSTER) {
        set_color(FOREGROUND_RED);
        kprintf("ERR: the rood directory was not loaded properly (missing EOF)");
        _panic();
    }
    return root;
}

void ls() {
    // load the root directory from its location
    // within the virtual file system
    folder_t *root = load_root_folder();

    // print out all files in it
    uint32_t i;
    for (i = 0; i < root->file_count; i++) {
        set_color(FOREGROUND_LIGHTGRAY);
        kprintf("NAME: ");
        reset_color();
        kprintf("%s ", root->files[i].name);

        set_color(FOREGROUND_LIGHTGRAY);
        kprintf("SIZE: ");
        reset_color();
        kprintf("%d [B] ", root->files[i].size);

        set_color(FOREGROUND_LIGHTGRAY);
        kprintf("CLUSTER: ");
        reset_color();
        kprintf("%d ", root->files[i].start_cluster_index);

        set_color(FOREGROUND_LIGHTGRAY);
        kprintf("SYS: ");
        reset_color();
        kprintf("%d ", root->files[i].system);

        set_color(FOREGROUND_LIGHTGRAY);
        kprintf("STATUS: ");
        reset_color();

        switch (root->files[i].open) {
            case 0:
                kprintf("CLOSED\n\r");
                break;
            case 1:
                kprintf("OPENED\n\r");
                break;
            default:
                kprintf("UNDEFINED\n\r");
        }
    }
    // deallocate it since it's not needed anymore
    free_root_dir(root);
}

int touch(char *filename) {
    // normalize the length of the file
    // to be touched
    normalize_filename(filename);

    // load the root directory
    folder_t *root = load_root_folder();

    // make sure the name isn't already taken
    if (exists_file(filename) == 1){
        free_root_dir(root);
        return 1;
    }

    // create a new file and store it into
    // the root directory
    create_file(root, filename);

    // clear out all clusters currently held
    // by the root directory, so it could be
    // restored again with it's updated size
    free_all_occupied_clusters(ROOT_FIRST_START_CLUSTER);
    save_root_folder(root);

    // deallocate it since it's not needed anymore
    free_root_dir(root);
    return 0;
}

static void create_file(folder_t *root, const char *filename) {
    // make sure there are at least 2 free clusters
    // (the start one of the file + its EOF cluster)
    if (exists_n_free_clusters(2) == 0) {
        kprintf("there is not enough space to create a file");
        return;
    }

    // create a new file array of a size of the current number of files + 1
    // and store there all files from the root directory
    file_t *files = (file_t *)kmalloc((root->file_count + 1) * sizeof(file_t));
    uint32_t i;
    for (i = 0; i < root->file_count; i++)
        memcpy(&files[i], &root->files[i], sizeof(file_t));

    // create the new file and store it at the last position within the array
    strcpy(files[root->file_count].name, filename);
    files[root->file_count].size = 0;
    files[root->file_count].open = 0;
    files[root->file_count].system = 0;
    files[root->file_count].start_cluster_index = get_free_cluster();

    // create an EOF cluster and link it up to the new file's start cluster
    uint32_t eof_cluster = get_free_cluster();
    fat[files[root->file_count].start_cluster_index].value = eof_cluster;
    fat[eof_cluster].value = EOF_CLUSTER;

    // deallocate the original files of the root directory
    kfree(root->files);

    // update the current directory
    root->files = files;
    root->file_count++;
}

int delete_system_file(char *filename) {
    // normalize the name of the file
    normalize_filename(filename);

    // load the root dir from the memory
    folder_t *root = load_root_folder();

    // make sure the file to be deleted DOES exist
    if (exists_file(filename) == 0) {
        free_root_dir(root);
        return 1;
    }
    // delete the file from the root directory
    delete_file(root, filename);

    // clear out all clusters currently held
    // by the root directory, so it could be
    // restored again with it's updated size
    free_all_occupied_clusters(ROOT_FIRST_START_CLUSTER);
    save_root_folder(root);

    // deallocate it since it's not needed anymore
    free_root_dir(root);
    return 0;
}

int rm(char *filename) {
    // normalize the name of the file
    normalize_filename(filename);

    // load the root dir from the memory
    folder_t *root = load_root_folder();

    file_t *file = get_file(root, filename);
    if (file->system == 1) {
        free_root_dir(root);
        return 1;
    }

    // make sure the file to be deleted DOES exist
    if (exists_file(filename) == 0) {
        free_root_dir(root);
        return 1;
    }
    // delete the file from the root directory
    delete_file(root, filename);

    // clear out all clusters currently held
    // by the root directory, so it could be
    // restored again with it's updated size
    free_all_occupied_clusters(ROOT_FIRST_START_CLUSTER);
    save_root_folder(root);

    // deallocate it since it's not needed anymore
    free_root_dir(root);
    return 0;
}

static void delete_file(folder_t *root, const char *filename) {
    uint32_t i, j;
    uint32_t file_pos;

    // find the file's position within the root directory
    for (file_pos = 0; file_pos < root->file_count; file_pos++)
        if (strcmp(root->files[file_pos].name, filename) == 0)
            break;

    // create a new file array of a reduced size (by 1)
    // and store there all the files of the root dir
    // except for the one to be deleted
    file_t *files = (file_t *)kmalloc((root->file_count - 1) * sizeof(file_t));
    for (i = 0, j = 0; i < root->file_count; i++) {
        if (i == file_pos)
            continue;
        memcpy(&files[j++], &root->files[i], sizeof(file_t));
    }

    // free all clusters held by the files
    // also we must explicitly free the start cluster of the file
    free_all_occupied_clusters(root->files[file_pos].start_cluster_index);
    fat[root->files[file_pos].start_cluster_index].value = FREE_CLUSTER;

    // deallocate the original files
    // and update the root directory
    kfree(root->files);
    root->files = files;
    root->file_count--;
}

static file_t *get_file(folder_t *root, char *filename) {
    // return a reference to a file
    // in the root dir given by its name
    normalize_filename(filename);
    uint32_t i;
    for (i = 0; i < root->file_count; i++)
        if (strcmp(root->files[i].name, filename) == 0)
            return &root->files[i];
    return NULL;
}

int file_exists(char *filename) {
    normalize_filename(filename);
    folder_t *root = load_root_folder();
    file_t *file = get_file(root, filename);
    free_root_dir(root);
    return file != NULL;
}

int cat(char *filename) {
    // normalize the length of the file and load the root dir
    normalize_filename(filename);
    folder_t *root = load_root_folder();

    // get the target file and make sure the file exists
    file_t *file = get_file(root, filename);
    if (file == NULL) {
        kprintf("file not found\n\r");
        free_root_dir(root);
        return 1;
    }
    // store the crucial information about the file
    // and deallocate the root directory as we'll not
    // need it (we only need the clusters -> fat)
    uint32_t curr_cluster = file->start_cluster_index;
    uint32_t size = file->size;
    free_root_dir(root);

    uint32_t offset = 0;     // offset within the buffer to be printed out
    uint32_t bytes_to_read;  // number of bytes to read from the current cluster
    uint32_t read_bytes = 0; // read bytes so far

    // iterate through the clusters until you reach the EOF cluster
    while (fat[curr_cluster].value != EOF_CLUSTER) {
        // check if we still can read the entire cluster
        // of just a portion of it (the last one)
        if (size - read_bytes >= CLUSTER_SIZE)
            bytes_to_read = CLUSTER_SIZE;
        else
            bytes_to_read = size - read_bytes;

        // check if it's time to flush (print out) the
        // content of the buffer
        if (offset + bytes_to_read >= SCREEN_BUFFER_SIZE) {
            file_buffer[offset] = '\0';
            kprintf("%s", file_buffer);
            offset = 0;
        }

        // read the data from the cluster into the buffer
        memcpy(&file_buffer[offset], (void *)CLUSTER_ADDR(curr_cluster), bytes_to_read);
        offset += bytes_to_read;
        read_bytes += bytes_to_read;

        // move on to the next cluster
        curr_cluster = fat[curr_cluster].value;
    }
    // print the entire buffer onto the screen
    file_buffer[offset] = '\0';
    kprintf("%s", file_buffer);
    return 0;
}

static void append_data(char *filename, char *buffer, uint32_t bytes) {
    // normalize the length of the file
    // and load the root directory
    normalize_filename(filename);
    folder_t *root = load_root_folder();

    // make sure the file we're going to append to exists
    file_t *file = get_file(root, filename);
    if (file == NULL) {
        kprintf("file not found\n\r");
        free_root_dir(root);
    }
    // offset within the last cluster (where the file data ends)
    uint32_t offset_in_last_cluster = file->size % CLUSTER_SIZE;

    // start cluster of the file
    uint32_t curr_cluster = file->start_cluster_index;

    // iterate to the last cluster of the file (the one before EOF_CLUSTER)
    while (fat[fat[curr_cluster].value].value != EOF_CLUSTER)
        curr_cluster = fat[curr_cluster].value;

    // calculate how many bytes we can store into the last cluster
    // (if it's not entirely full)
    uint32_t bytes_in_last_cluster = min(bytes, (CLUSTER_SIZE - offset_in_last_cluster));

    // copy the bytes into the last cluster of the file and update the file size
    memcpy((void *)(CLUSTER_ADDR(curr_cluster) + offset_in_last_cluster), buffer, bytes_in_last_cluster);
    file->size += bytes_in_last_cluster;

    // if we don't have any other bytes to store, we're done
    // (we have to re-store the root dir - there's the updated file size)
    if (bytes <= bytes_in_last_cluster) {
        free_all_occupied_clusters(ROOT_FIRST_START_CLUSTER);
        save_root_folder(root);
        free_root_dir(root);
        return;
    }

    // make sure we have enough clusters available to store the rest of the file as well
    uint32_t cluster_needed = get_cluster_count_needed(bytes - bytes_in_last_cluster);
    if (exists_n_free_clusters(cluster_needed) == 0) {
        kprintf("no enough space to store the rest of the file\n\r");
        return;
    }
    // set the EOF cluster as free, so it could be used to store data
    fat[fat[curr_cluster].value].value = FREE_CLUSTER;

    file->size += bytes - bytes_in_last_cluster;
    uint32_t prev_cluster = curr_cluster;
    uint32_t bytes_stored = bytes_in_last_cluster;

    // the very last cluster needs to be dealt with
    // separately as it's not fully occupied with data
    // (we need to calculate how many bytes we store into it)
    uint32_t i;
    for (i = 0; i < cluster_needed - 1; i++) {
        // create a link in the FAT table
        curr_cluster = get_free_cluster();
        fat[prev_cluster].value = curr_cluster;
        prev_cluster = curr_cluster;

        // store data into the current cluster
        memcpy((void *)CLUSTER_ADDR(curr_cluster), &buffer[bytes_stored], CLUSTER_SIZE);

        // update how many bytes we've stored so far
        bytes_stored += CLUSTER_SIZE;
    }
    // create a link in the FAT table
    curr_cluster = get_free_cluster();
    fat[prev_cluster].value = curr_cluster;

    // store the very last bytes into the very last cluster
    memcpy((void *)CLUSTER_ADDR(curr_cluster), &buffer[bytes_stored], bytes - bytes_stored);

    // create an EOF cluster and link it up to the rest of the chain
    prev_cluster = curr_cluster;
    curr_cluster = get_free_cluster();
    fat[prev_cluster].value = curr_cluster;
    fat[curr_cluster].value = EOF_CLUSTER;

    // re-store the root directory
    // so the file has its updated size
    free_all_occupied_clusters(ROOT_FIRST_START_CLUSTER);
    save_root_folder(root);
    free_root_dir(root);
}

int cp(char *src, char *des) {
    // normalize the file names
    // and make sure they're not the same file
    normalize_filename(src);
    normalize_filename(des);
    if (strcmp(src, des) == 0)
        return 1;

    // make sure the source file exists
    folder_t *root = load_root_folder();
    file_t *src_file = get_file(root, src);
    if (src_file == NULL) {
        // kprintf("source file not found\n\r");
        free_root_dir(root);
        return 1;
    }
    // store the start cluster of the source file
    uint32_t src_curr_cluster = src_file->start_cluster_index;

    // if the destination file already exists, delete it
    file_t *des_file = get_file(root, des);
    free_root_dir(root);
    if (des_file != NULL)
        rm(des);

    // create a destination file
    touch(des);

    // make sure we have enough free clusters to store the file
    if (exists_n_free_clusters(get_cluster_count_needed(src_file->size)) == 0) {
        set_color(FOREGROUND_LIGHTRED);
        kprintf("ERR: there is not enough place to store the file\n\r");
        reset_color();
        return 1;
    }

    // load the brand-new destination file
    // and store its start cluster, also
    // update its size and restore the root dir
    // so the change takes effect
    root = load_root_folder();
    des_file = get_file(root, des);
    uint32_t des_prev_cluster;
    uint32_t des_curr_cluster = des_file->start_cluster_index;
    des_file->size = src_file->size;
    free_all_occupied_clusters(ROOT_FIRST_START_CLUSTER);
    save_root_folder(root);
    free_root_dir(root);

    // set the EOF cluster of the new file as free,
    // so it could be used to store data
    fat[fat[des_curr_cluster].value].value = FREE_CLUSTER;

    // keep copying clusters until you reach the end of the source file
    while (fat[src_curr_cluster].value != EOF_CLUSTER) {
        // copy the current cluster into the destination one
        memcpy((void *)CLUSTER_ADDR(des_curr_cluster), (void *)CLUSTER_ADDR(src_curr_cluster), CLUSTER_SIZE);

        // create a link in the FAT table (destination file)
        des_prev_cluster = des_curr_cluster;
        des_curr_cluster = get_free_cluster();
        fat[des_prev_cluster].value = des_curr_cluster;

        // move on to the next pair of clusters
        src_curr_cluster = fat[src_curr_cluster].value;
    }
    // mark the last cluster of the destination file as an EOF cluster
    fat[des_curr_cluster].value = EOF_CLUSTER;
    return 0;
}

void print_FAT(uint32_t n) {
    uint32_t i;
    uint32_t to = min(n, CLUSTER_COUNT);
    for (i = 0; i < to; i++) {
        switch (fat[i].value) {
            case EOF_CLUSTER:
                set_color(FOREGROUND_YELLOW);
                kprintf("E ");
                reset_color();
                break;
            case FREE_CLUSTER:
                set_color(FOREGROUND_GREEN);
                kprintf("F ");
                reset_color();
                break;
            case TAKEN_CLUSTER:
                set_color(FOREGROUND_BLUE);
                kprintf("T ");
                reset_color();
                break;
            default:
                kprintf("%d ", fat[i].value);
                break;
        }
    }
    kprintf("\n\r");
}

int read(char *filename, char *buffer, uint32_t offset, uint32_t len) {
    // normalize the filename, load the root directory
    // and get the file, so we know where the file starts and how bit it is
    normalize_filename(filename);
    folder_t *root = load_root_folder();
    file_t *file = get_file(root, filename);
    free_root_dir(root);

    // store some essential information about the file
    // (we'll be needing them)
    uint32_t curr_cluster = file->start_cluster_index;
    uint32_t size = file->size;

    // make sure we don't want to read out of the
    // boundaries of the file
    if ((offset + len) > size) {
        kprintf("the start byte is out of range\n\r");
        return 1;
    }

    // calculate the start cluster we should read off of
    // as well as the offset we start at within that cluster
    uint32_t start_cluster = offset / CLUSTER_SIZE;
    uint32_t offset_in_start_cluster = offset % CLUSTER_SIZE;

    // move on to the first cluster we want to read data out of
    uint32_t i;
    for (i = 0; i < start_cluster; i++)
        curr_cluster = fat[curr_cluster].value;

    // read the data from the first cluster
    uint32_t bytes_read = min(len, CLUSTER_SIZE - offset_in_start_cluster);
    memcpy(buffer, (void *)(CLUSTER_ADDR(curr_cluster) + offset_in_start_cluster), bytes_read);

    // if we don't have to read any more data, we're done here
    if (len <= (CLUSTER_SIZE - offset_in_start_cluster))
        return 1;

    // keep reading data from clusters until
    // all bytes have been read
    uint32_t bytes_to_read;
    while (bytes_read != len) {
        // move on to the next cluster using the FAT table
        curr_cluster = fat[curr_cluster].value;

        // check how many bytes we can read
        if (bytes_read + CLUSTER_SIZE <= len)
            bytes_to_read = CLUSTER_SIZE;
        else
            bytes_to_read = len - bytes_read;

        // read the amount of bytes from the current cluster
        // and update how many bytes we've read so far
        memcpy(&buffer[bytes_read], (void *)CLUSTER_ADDR(curr_cluster), bytes_to_read);
        bytes_read += bytes_to_read;
    }
    return 0;
}

int is_file_open(char *filename) {
    // normalize the length of the file,
    // load the root dir, so we can get the file,
    // and finally load up the file
    normalize_filename(filename);
    folder_t *root = load_root_folder();
    file_t *file = get_file(root, filename);

    // free the root dir as we don't need it
    free_root_dir(root);

    // if the file doesn't exist of it has not been opened
    // return 0, otherwise return 1
    if (file == NULL || file->open == 0)
        return 0;
    return 1;
}

int set_as_system_file(char *filename) {
    // check if the file is indeed closed
    normalize_filename(filename);
    if (is_file_open(filename) == 1)
        return 1; // error

    folder_t *root = load_root_folder();
    file_t *file = get_file(root, filename);
    file->system = 1;
    free_all_occupied_clusters(ROOT_FIRST_START_CLUSTER);
    save_root_folder(root);
    free_root_dir(root);

    return 0; // success
}

int open_file(char *filename) {
    // check if the file is indeed closed
    normalize_filename(filename);
    if (is_file_open(filename) == 1)
        return 1; // error

    // set the flag that the file is now opened
    // and save the root dir so the change takes effect
    folder_t *root = load_root_folder();
    file_t *file = get_file(root, filename);
    file->open = 1;
    free_all_occupied_clusters(ROOT_FIRST_START_CLUSTER);
    save_root_folder(root);
    free_root_dir(root);

    return 0; // success
}

int close_file(char *filename) {
    // check if the file is indeed opened
    normalize_filename(filename);
    if (is_file_open(filename) == 0)
        return 1; // error

    // set the flag that the file is now closed
    // and save the root dir so the change takes effect
    folder_t *root = load_root_folder();
    file_t *file = get_file(root, filename);
    file->open = 0;
    free_all_occupied_clusters(ROOT_FIRST_START_CLUSTER);
    save_root_folder(root);
    free_root_dir(root);

    return 0; // success
}

int write(char *filename, char *buffer, uint32_t offset, uint32_t len) {
    // normalize the name of the file and get the corresponding file
    normalize_filename(filename);
    folder_t *root = load_root_folder();
    file_t *file = get_file(root, filename);

    // make sure the file does exist
    if (file == NULL) {
        kprintf("file not found\n\r");
        free_root_dir(root);
        return 1;
    }
    // make sure the offset falls into the files boundaries
    if (offset > file->size) {
        kprintf("the offset is greater than the size of the file itself\n\r");
        free_root_dir(root);
        return 1;
    }
    // check if attaching the file to the end would be enough
    if (offset == file->size) {
        free_root_dir(root);
        append_data(filename, buffer, len);
        return 0;
    }
    // make sure we have enough clusters available to store the contents of the file
    if (exists_n_free_clusters(get_cluster_count_needed(file->size + len)) == 0) {
        kprintf("not enough space to extend the file\n\r");
        free_root_dir(root);
        return 1;
    }
    // store the original size of the file
    uint32_t original_file_size = file->size;

    // the size of the file is the offset (where we have to cut the file off)
    // re-store the root directory so the file has its updated size
    file->size = offset;
    free_all_occupied_clusters(ROOT_FIRST_START_CLUSTER);
    save_root_folder(root);
    free_root_dir(root);

    // calculate the cluster we want to insert data into
    // as well as the offset within that cluster
    uint32_t curr_cluster = file->start_cluster_index;
    uint32_t start_cluster = offset / CLUSTER_SIZE;
    uint32_t offset_in_start_cluster = offset % CLUSTER_SIZE;

    // calculate the size of the tmp buffer we'll need to store
    // the rest of hte file (from where it'll be cut off)
    uint32_t clusters_needed = get_cluster_count_needed(original_file_size);
    uint32_t tmp_buffer_size = (clusters_needed - start_cluster) * CLUSTER_SIZE;

    // create a tmp array for the tail of the file that had to cut off
    // in order to insert there the new data
    uint32_t tmp_buff_offset = 0;
    char *tmp_buff = (char *)kmalloc(tmp_buffer_size);
    memset((void *)tmp_buff, 0, tmp_buffer_size);

    // move to the first cluster we want to copy
    // data into the tmp array (so it could be appended
    // to the end once we're inserted the data)
    uint32_t i;
    for (i = 0; i < start_cluster; i++)
        curr_cluster = fat[curr_cluster].value;

    // the amount of bytes we copy from the first (last) cluster
    // (the spot were we split the file up)
    uint32_t bytes_in_first_cluster = CLUSTER_SIZE - offset_in_start_cluster;

    // copy the amount of bytes from the first (last) cluster into our tmp array
    memcpy((void *)tmp_buff, (void *)(CLUSTER_ADDR(curr_cluster) + offset_in_start_cluster), bytes_in_first_cluster);
    tmp_buff_offset += bytes_in_first_cluster;

    // move on to the next cluster
    uint32_t first = 1;
    uint32_t prev_cluster;
    curr_cluster = fat[curr_cluster].value;

    // keep copying cluster until you reach the end (then we'll have the split up part of
    // the file store in our tmp array)
    while (fat[curr_cluster].value != EOF_CLUSTER) {
        // copy data from the current cluster
        memcpy((void *)&tmp_buff[tmp_buff_offset], (void *)CLUSTER_ADDR(curr_cluster), CLUSTER_SIZE);
        tmp_buff_offset += CLUSTER_SIZE;

        // move on to the next cluster
        prev_cluster = curr_cluster;
        curr_cluster = fat[curr_cluster].value;

        // set the cluster as free, so it could be used again
        // when appending data (the one after the split spot needs
        // to be set as an EOF cluster, so the append function can
        // determinate the end of the file - where to append data)
        if (first == 1) {
            fat[prev_cluster].value = EOF_CLUSTER;
            first = 0;
        } else {
            fat[prev_cluster].value = FREE_CLUSTER;
        }
    }
    // append the data we want to insert
    append_data(filename, buffer, len);
    // append the original tail of the file (what we split off)
    append_data(filename, tmp_buff, tmp_buff_offset - (CLUSTER_SIZE - original_file_size % CLUSTER_SIZE));

    // we don't need to the tmp array anymore
    kfree(tmp_buff);
    return 0;
}

uint32_t get_file_size(char *filename) {
    normalize_filename(filename);
    folder_t *root = load_root_folder();
    file_t *file = get_file(root, filename);
    if (file == NULL)
        return 0;
    uint32_t size = file->size;
    free_root_dir(root);
    return size;
}

uint32_t get_memory_available() {
    uint32_t free_cluster = 0;
    uint32_t i;
    for (i = 0; i < CLUSTER_COUNT; i++)
        free_cluster += (fat[i].value == FREE_CLUSTER);
    return free_cluster * CLUSTER_SIZE;
}