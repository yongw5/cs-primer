/* System V semaphore 
 * int semget(key_t key, int nsems, int oflag);
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)
#define SVSEM_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

int main(int argc, char **argv) {
  int opt, oflag, semid, nsems;
  key_t key;

  oflag = SVSEM_MODE | IPC_CREAT;
  while((opt = getopt(argc, argv, "e")) != -1) {
	switch(opt) {
	  case 'e':
		oflag |= IPC_EXCL;
		break;
	}
  }
  if(optind != argc -2) {
	printf("usage: %s [-e] <pathname> <nsems>\n", argv[0]);
	exit(1);
  }
  nsems = atoi(argv[optind+1]);
  if((key = ftok(argv[optind], 0)) == -1)
	error_handler(ftok error);
  if((semid = semget(key, nsems, oflag)) == -1)
	error_handler(semget error);
  return 0;
}
