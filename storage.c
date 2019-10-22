#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "storage.h"

#define BLOCK_SIZE  512 // bytes in a block
#define BLOCK_COUNT 256 // blocks in the disk

#define NVM_LEN   60000 // bytes of nonvolatile memory available

#define FILENAME_DISK "disk.bin"
#define FILENAME_NVM  "nvm.bin"

FILE* _file_disk = NULL;
FILE* _file_nvm  = NULL;

bool _initialized = false;

void _storage_init() {
    if (!_initialized) {
        // Seed the random number generator.
        srand(time(0));

        // Create/open the needed files.
        _file_disk = fopen(FILENAME_DISK, "r+b");
        if (_file_disk == NULL) {
            // Have to create the file first.
            _file_disk = fopen(FILENAME_DISK, "wb");
            if (_file_disk == NULL) {
                fprintf(stderr, "Could not open " FILENAME_DISK "\n");
                perror("Error printed by perror");
                exit(-1);
            }
            // Make the file the correct size.
            fseek(_file_disk, (BLOCK_SIZE*BLOCK_COUNT)-1, SEEK_SET);
            fputc('\0', _file_disk);
        }
        fseek(_file_disk, 0, SEEK_SET);

        _file_nvm = fopen(FILENAME_NVM, "r+b");
        if (_file_nvm == NULL) {
            // Have to create the file first.
            _file_nvm = fopen(FILENAME_NVM, "wb");
            if (_file_nvm == NULL) {
                fprintf(stderr, "Could not open " FILENAME_NVM "\n");
                perror("Error printed by perror");
                exit(-1);
            }
            // Make the file the correct size.
            fseek(_file_nvm, NVM_LEN-1, SEEK_SET);
            fputc('\0', _file_nvm);
        }
        fseek(_file_nvm, 0, SEEK_SET);

        _initialized = true;
    }
}

void _delay(int ms) {
    clock_t start_time = clock();
    while (clock() < start_time + ms);
}

void _random_delay(int ms) {
    int jitter = (rand() % 10) - 5;
    _delay(ms + jitter);
}

int disk_write(int block_number, char* buffer) {
    _storage_init();

    if (block_number >= BLOCK_COUNT) return -1;

    _random_delay(15);

    fseek(_file_disk, block_number*BLOCK_SIZE, SEEK_SET);
    fwrite(buffer, 1, BLOCK_SIZE, _file_disk);

    return 0;
}

int disk_read(int block_number, char* buffer) {
    _storage_init();

    if (block_number >= BLOCK_COUNT) return -1;

    _random_delay(10);

    fseek(_file_disk, block_number*BLOCK_SIZE, SEEK_SET);
    fread(buffer, 1, BLOCK_SIZE, _file_disk);

    return 0;
}

int disk_block_count() {
    return BLOCK_COUNT;
}

int nvm_write(int byte_number, int length, char* buffer) {
	_storage_init();
    if (byte_number+length > NVM_LEN) return -1;

    fseek(_file_nvm, byte_number, SEEK_SET);
    fwrite(buffer, 1, length, _file_nvm);

    return 0;
}

int nvm_read(int byte_number, int length, char* buffer) {
	_storage_init();
    if (byte_number+length > NVM_LEN) return -1;

    fseek(_file_nvm, byte_number, SEEK_SET);
    fread(buffer, 1, length, _file_nvm);

    return 0;
}

int nvm_byte_count() {
    return NVM_LEN;
}
