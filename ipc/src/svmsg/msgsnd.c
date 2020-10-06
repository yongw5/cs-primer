/**
 * compiler gcc msgsnd.c -o msgsnd -D_GNU_SOURCE
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/errno.h>

#define MSG_W 0400
#define error_handler(msg) \
  do{ \
    printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
    exit(1); \
  }while(0)

int main(int argc, char **argv) {
  int mqid;
  size_t len;
  long type;
  struct msgbuf *ptr;
  key_t key;

  if(argc != 4) {
	printf("usage: %s <pathname> <#bytes> <type>\n", argv[0]);
	exit(1);
  }

  len = atoi(argv[2]);
  type = atoi(argv[3]);
  if((key = ftok(argv[1], 0)) == -1)
	error_handler(ftok error);
  if((mqid = msgget(key, MSG_W)) == -1)
	error_handler(msgget error);
  if((ptr = calloc(sizeof(long) + len, sizeof(char))) == NULL)
	error_handler(malloc error);
  ptr->mtype = type;
  if(msgsnd(mqid, ptr, len, 0) == -1)
	error_handler(msgsend error);
  return 0;
}
