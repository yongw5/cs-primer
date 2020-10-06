#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/errno.h>

#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(1);

int main(int argc, char **argv) {
  int mqid;
  key_t key;
  if(argc != 2) {
	printf("usage: %s <pathname>\n", argv[0]);
	exit(1);
  }
  if((key = ftok(argv[1], 0)) == -1)
	error_handler(ftok error);
  if((mqid = msgget(key, 0)) == -1)
	error_handler(msgget error);
  msgctl(mqid, IPC_RMID, NULL);
  return 0;
}
