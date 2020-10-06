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
#define SERV_FIFO "/tmp/fifo.serv"
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

void error_handler(char *msg);

int main(int argc, char **argv) {
  int readfd, writefd, fd;
  char *ptr, buf[MAXSIZE+1], fifoname[MAXSIZE];
  pid_t pid;
  ssize_t n;

  /* create server's well-know FIFO, ok if already exists */
  if((mkfifo(SERV_FIFO, FILE_MODE) == -1) && (errno != EEXIST))
	error_handler("mkfifo() error");
  if((readfd = open(SERV_FIFO, O_RDONLY)) == -1)
	error_handler("cannot open server fifo to read");

  while(read(readfd, buf, MAXSIZE) > 0) {
	n = strlen(buf);
	if(buf[n-1] == '\n') {
	  buf[n-1] = 0;
	  --n;
	}
	if((ptr = strchr(buf, ' ')) == NULL) {
	  printf("bogus request: %s", buf);
	  continue;
	}
	*ptr++ = 0;
	pid = atol(buf);
	snprintf(fifoname, sizeof(fifoname), "/tmp/fifo.%ld", (long)pid);
	if((writefd = open(fifoname, O_WRONLY)) == -1)
	  error_handler("cannot open client file to write");
	if((fd = open(ptr, O_RDONLY)) == -1) {
	  snprintf(buf+n, MAXSIZE-n, ": cannot open, %s\n", strerror(errno));
	  write(writefd, ptr, strlen(ptr));
	  close(writefd);
	} else {
	  char tmp[] = "the context of file: \n";
	  write(writefd, tmp, sizeof(tmp));
	  while((n = read(fd, buf, MAXSIZE)) > 0) 
		write(writefd, buf, n);
	  close(fd);
	  close(writefd);
	}
  }
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
