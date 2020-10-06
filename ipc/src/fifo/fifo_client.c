/**
 * single server and multi-clients using fifo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#define MAXSIZE 256
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define SERV_FIFO "/tmp/fifo.serv"
void error_handler(char *msg);

int main(int argc, char **argv) {
  int readfd, writefd;
  size_t len;
  ssize_t n;
  char *ptr, fifoname[MAXSIZE], buf[MAXSIZE];
  pid_t pid;

  pid = getpid();
  snprintf(fifoname, sizeof(fifoname), "/tmp/fifo.%ld", (long)pid);
  if((mkfifo(fifoname, FILE_MODE) == -1) && (errno != EEXIST))
	error_handler("can't create clente fifo file");

  /* start buffer with pid and a blank */
  snprintf(buf, sizeof(buf), "%ld ", (long)pid);
  len = strlen(buf);
  ptr = buf + len;
  
  fputs("input pathname: ", stdout);
  fgets(ptr, MAXSIZE-len, stdin);

  if((writefd = open(SERV_FIFO, O_WRONLY)) == -1) 
	error_handler("can't open server fifo file");
  write(writefd, buf, sizeof(buf));

  if((readfd = open(fifoname, O_RDONLY)) == -1)
	error_handler("cannot open client fifo to read");
  while((n = read(readfd, buf, MAXSIZE)) > 0) {
	buf[n] = 0;
	fputs(buf, stdout);
  }
  close(readfd);
  unlink(fifoname);
  close(writefd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
