#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/errno.h>

#define MQ_KEY 1234L
#define MAXMSG (8192 + sizeof(long))
#define MSG_R 0400
#define MSG_W 0200
#define SVMSG_MODE (MSG_R | MSG_W | MSG_R>>3 | MSG_W>>6)
#define error_handler(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)
void client(int, int);

int main(int argc, char **argv) {
  int readfd, writefd;
  if((writefd = msgget(MQ_KEY, 0)) == -1)
	error_handler(msgget error);
  if((readfd = msgget(IPC_PRIVATE, SVMSG_MODE | IPC_CREAT)) == -1)
	error_handler(msgget error);
  client(readfd, writefd);
  return 0;
}

void client(int readfd, int writefd) {
  size_t len;
  ssize_t n;
  char *ptr;
  struct msgbuf *buf;
  long type;

  if((buf = malloc(MAXMSG)) == NULL)
	error_handler(malloc error);

  snprintf(buf->mtext, MAXMSG, "%ld ", readfd); /* satrt buffer with mqid and a blank */
  len = strlen(buf->mtext);
  ptr = buf->mtext + len;
  printf("input pathname: ");
  fgets(ptr, MAXMSG-len, stdin);
  len = strlen(buf->mtext);
  if(buf->mtext[len-1] == '\n')
	--len;
  buf->mtype = 1;
  if(msgsnd(writefd, buf, len, 0) == -1) { /* write mqid and pathname to server's well-knwn queue */
	free(buf);
	error_handler(msgsnd error);
  }

  /* read from our queue, write to standard output */
  while((n = msgrcv(readfd, buf, MAXMSG, 1, 0)) > 0)
	write(STDIN_FILENO, buf->mtext, n);
  free(buf);
}
