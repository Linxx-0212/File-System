#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "storage.h"

#define BLOCK_COUNT 256
#define NVM_COUNT 234

typedef struct List {
	int block;
	int location;
	struct List* next;
}List;

typedef struct File {
	char name[512];
	List* start_block;
	bool r_or_w;
	int status;
	int pointer;
}File;

File files[500];
bool blocks[BLOCK_COUNT];
bool nvm_blocks[NVM_COUNT];

int initialize = false;

int get_id(int mode) {
  // TODO: Make atomic if multiplexing green threads onto OS threads.
    static int number = -1;
    if (mode == 2) {
    	number = -1;
    	return 0;
    }
    if (mode == 1) {
    	return number;
    }
    else
    	return ++number;
}

void initial() {
	for (int i = 0; i <BLOCK_COUNT; i++) {
		blocks[i] = true;
	}
	for (int i = 0; i < NVM_COUNT; i++) {
		nvm_blocks[i] = true;
	}
	get_id(2);
	initialize = true;
}

int get_next_block() {
	for (int i = 0; i < BLOCK_COUNT; i++) {
		if (blocks[i] ==  true) {
			blocks[i] = false;
			return i;
		}
	}
	return -1;
}

int get_nvm_block() {
	for (int i = 0; i < NVM_COUNT; i++ ) {
		if (nvm_blocks[i] == true) {
			nvm_blocks[i] = false;
			return i;
		}
	}
	return -1;
}

void free_blocks(List* node) {
	if (node == NULL)
		return;
	free_blocks(node->next);
	if (node->location == 1)
		blocks[node->block] = true;
	else
		nvm_blocks[node->block] = true;
	free(node);
	return;
}



int uva_open(char* filename, bool writeable) {

	if (!initialize)
		initial();
	//printf("%d\n", get_id(1));
	for (int i = 0; i <=get_id(1); i++) {
		if (strcmp(files[i].name,filename) == 0){
			files[i].r_or_w = writeable;
			files[i].status = 1;
			files[i].pointer = 0;
			return i;
		}
	}
	File new_file;
	//new_file.name = malloc(sizeof(filename));
	strcpy(new_file.name,filename);
	new_file.r_or_w = writeable;
	new_file.start_block = NULL;
	new_file.status = 1;
	new_file.pointer = 0;
	files[get_id(0)] = new_file;
	return get_id(1);

}

int uva_close(int file_identifier) {
	files[file_identifier].status = 0;

	return 0;
}

int uva_read(int file_identifier, char* buffer, int offset, int length) {
	File cur = files[file_identifier];

	//printf("%d",cur.status);
	if (cur.status == 0 || cur.r_or_w == true) 
		return -1;
	int begin = 0;
	int size = 0;
	List* start = cur.start_block;
	if (start->location == 0)
		size = 256;
	else
		size = 512;
	while (begin + size < cur.pointer) {
		start = start->next;
		begin += size;
	}
	char tmp[length+10];
	int count = 0;
	begin = cur.pointer - begin;
	begin += offset;

	if (begin >= size) {
		begin -= size;
		start = start->next;
		if (start == NULL)
			return count;
		files[file_identifier].pointer += offset;
	}
	while (count < length) {
		if (start->location == 0)
			size = 256;
		else
			size = 512;
		char cache[size];
		if (start->location == 0)
			nvm_read(start->block*256,256,cache);
		else
			disk_read(start->block,cache);
		//printf("%s",cache);
		int len = 0;
		
		if (strlen (cache) < size)
			len = strlen(cache)-begin;
		else
			if ((length-count) > (size - begin))
				len = size-begin;
			else
				if ((length - count) < strlen(cache)-begin)
					len = length - count;
		for (int i = begin; i < begin + len; i++) {
			tmp[i+count-begin] = cache[i];
		}
		// printf("%s\n",cache);
		// printf("%d\n", len);
		count+=len;
		//buffer[count + 1] = '\0';
		// printf("count:%d\n",count);
		// printf("len:%d\n",len);
		//printf("%s\n", cache);
		start = start->next;
		begin = 0;
		
		if (start == NULL|| count >= length) {
			int j = 0;
			// printf("%ld\n",strlen(cache));
			// printf("%d\n",count);
			// printf("%d\n",)
			tmp[count] = '\0';
			while (tmp[j] != '\0') {
				buffer[j] = tmp[j];
				j ++;
			}
			buffer[j] = '\0';
			// printf("%s\n", buffer);
			files[file_identifier].pointer += j;
			return j;
		}
	}
	return count;
}

int uva_read_reset(int file_identifier) {
	files[file_identifier].pointer = 0;
	return 0;
}

int uva_write(int file_identifier, char* buffer, int length) {
	// 
	length = strlen(buffer);
	// printf("%d\n",length);
	int size = 0;
	File cur = files[file_identifier];	
	//return cur.status;
	if (cur.status == 0 || cur.r_or_w == false)
		return -1;
	int index;
	//printf("%ld\n", strlen(buffer));
	//delete previor
	int location = 0;
	free_blocks(files[file_identifier].start_block);
	//printf("%s\n\n",buffer);
	if (cur.start_block == NULL) {
		if (length > 256) {
			index = get_next_block();
			location = 1;
			size = 512;
			if (index == -1)
				return -1;
		}
		else {
			index = get_nvm_block();
			location = 0;
			size = 256;
			if (index == -1) {
				index = get_next_block();
				location = 1;
				size = 512;
				if (index == -1)
					return -1;
			}
		}
	}

	List* new_block;
	char tmp[size];
	int len;
	if (length > size)
		len = size;
	else
		len = length;
	for (int i = 0; i < len; i++){
		tmp[i] = buffer[i];
	}

	new_block = malloc(sizeof(List));
	new_block->block = index;
	new_block->next = NULL;
	new_block->location = location;
	//printf("%d\n",index);
	cur.start_block = new_block;
	if (len < size) {
		tmp[len] = '\0';
		len ++;
	}
	//printf("%s",tmp)
	if (location == 0)
		nvm_write(index*256,len,tmp);
	else
		disk_write(index,tmp);
	length -= size;
	int count = size;
	while (length > 0) {
		//printf("length:%d\n",length);
		
		List* p = malloc(sizeof(List));
		location = 0;
		if (length > 512) {
			len = 512;
		}
		else
			len = length;
		if (len > 256) {
			index = get_next_block();
			location = 1;
			size = 512;
			if (index == -1)
				return -1;
		}
		else {
			index = get_nvm_block();
			size = 256;
			if (index == -1) {
				location = 1;
				index = get_next_block();
				size =512;
				if (index == -1)
					return -1;
			}
		}
		char tmp2[size];
		p->block = index;
		p->location = location;
		
		for (int i = 0; i < len; i++){
			tmp2[i] = buffer[count+i];
		}
		if (len < size) {
			tmp2[len] = '\0';
			len ++;
		}
		// printf("%s\n",tmp2);
		if (location == 0)
			nvm_write(index*256,len,tmp2);
		else
			disk_write(index,tmp2);
		new_block->next = p;
		new_block = new_block->next;
		new_block->next = NULL;
		length -= size;
		count += size;
	}
	files[file_identifier] = cur;
	return 0;
}
