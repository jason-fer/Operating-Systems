#include <stdio.h>
#include <sys/select.h>
#include "udp.h"
#include "mfs.h"


char *name = "tempfile";
int pinum, type, inum, block, response;
char buffer[4096];
MFS_Stat_t *my_dir;
MFS_Stat_t *my_file;

void test_init(){
	response = MFS_Init("localhost", 10000);
	assert(response >= 0);
	printf("MFS_Init passed\n");  
}

void test_creat(){
	response = MFS_Creat(pinum, type, name);
	assert(response >= 0);
	printf("MFS_Creat passed\n");
}

void test_write(){
	response = MFS_Write(inum, buffer, block);
	assert(response >= 0);
	printf("MFS_Write passed\n");
}

void test_read(){
	response = MFS_Read(inum, buffer, block);
	assert(response >= 0);
	printf("MFS_Read passed\n");
}

void test_stat(){
	response = MFS_Stat(inum, my_file);
	assert(response >= 0);
	printf("MFS_Stat passed\n");
}

void test_lookup(){
	response = MFS_Lookup(0, ".");
        printf("Inode value received = %d\n", response);
	assert(response >= 0);
	printf("MFS_Lookup passed\n");
}

void test_unlink(){
	response = MFS_Unlink(pinum, name);
	assert(response >= 0);
	printf("MFS_Unlink passed\n");
}

void test_shutdown(){
	response = MFS_Shutdown();
	assert(response >= 0);
	printf("MFS_Shutdown passed\n");
}

/**
 *  Test each respective client method..
 */
int main(int argc, char *argv[]){

	test_init();
	// test_creat();
	// test_write();
	// test_read();
	// test_stat();
	test_lookup();
	// test_unlink();
	// test_shutdown();

	char message[BUFFER_SIZE];
	sprintf(message, "hello world");

	return 0;
}

