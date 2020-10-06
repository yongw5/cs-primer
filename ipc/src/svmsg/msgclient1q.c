#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/errno.h>

#define MAXMSG (8192 + sizeof(long))
#define MQ_KEY 1234L
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)
void client(int, int);

int main(int argc, char **argv) {
  int mqid;
  if((mqid = msgget(MQ_KEY, 0)) == -1)
	error_handler(msgget error);
  client(mqid, mqid);
  return 0;
}

void client(int readfd, int writefd) {
  size_t len;
  ssize_t n;
  long type;
  char *ptr;
  struct msgbuf *buf;

  if((buf = malloc(MAXMSG)) == NULL)
	error_handler(malloc error);
  snprintf(buf->mtext, MAXMSG, "%ld ", (long)getpid());
  len = strlen(buf->mtext);
  ptr = buf->mtext + len;
  printf("input pathname: ");
  fgets(ptr, MAXMSG-len, stdin);
  len = strlen(buf->mtext);
  if(buf->mtext[len-1] == '\n')
	--len;
  buf->mtype = 1;
  if(msgsnd(writefd, buf, len, 0) == -1) {
	free(buf);
	error_handler(msgsnd error);
  }
  type = getpid();
  while((n = msgrcv(readfd, buf, MAXMSG, type, 0)) > 0)
	write(STDOUT_FILENO, buf->mtext, n);
  free(buf);
}
