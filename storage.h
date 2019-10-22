#pragma once

int disk_write(int block_number, char* buffer);
int disk_read(int block_number, char* buffer);
int disk_block_count();
int nvm_write(int byte_number, int length, char* buffer);
int nvm_read(int byte_number, int length, char* buffer);
int nvm_byte_count();
