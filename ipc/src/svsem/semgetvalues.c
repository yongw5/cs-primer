#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <errno.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};


int main(int argc, char **argv) {
  int semid, nsems, i;
  key_t key;
  struct semid_ds seminfo;
  union semun arg;

  if(argc != 2) {
	printf("usage: %s <pathname>\n", argv[1]);
	exit(1);
  }

  /* first get the number of semahpores in the set */
  if((key = ftok(argv[1], 0)) == -1)
	error_handler(ftok error);
  if((semid = semget(key, 0, 0)) == -1)
	error_handler(semget error);
  arg.buf = &seminfo;
  if(semctl(semid, 0, IPC_STAT, arg) == -1)
	error_handler(semctl error);
  nsems = arg.buf->sem_nsems;
  if((arg.array = calloc(nsems, sizeof(unsigned short))) == NULL)
	error_handler(calloc error);

  /* fetch the valuse and print*/
  if(semctl(semid, 0, GETALL, arg) == -1)
	error_handler(semctl error);
  for(i = 0; i < nsems; ++i)
	printf("semval[%d] = %d\n", i, arg.array[i]);
  free(arg.array);
  return 0;
}
