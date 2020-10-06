#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <errno.h>

#define SVSHM_MODE (SHM_R | SHM_W | SHM_R>>3 | SHM_W>>6)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int i, id;
  key_t key;
  struct shmid_ds buf;
  unsigned char c, *ptr;

  if(argc != 2) {
	printf("usage: %s <pathname>\n", argv[0]);
	exit(1);
  }

  if((key = ftok(argv[1], 0)) == -1)
	error_handler(ftok error);
  if((id = shmget(key, 0, SVSHM_MODE)) == -1)
	error_handler(shmget error);
  if((ptr = shmat(id, NULL, 0)) == (void*)-1)
	error_handler(shmat error);
  if(shmctl(id, IPC_STAT, &buf) == -1)
	error_handler(shmctl error);

  /* check that ptr[0] = 0, ptr[1] = 1, etc */
  for(i = 0; i < buf.shm_segsz; ++i)
	if((c = *ptr++) != (i % 256))
	  printf("ptr[%d] = %d\n", i, c);

  return 0;
}
