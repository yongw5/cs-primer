/**
 * 用FIFO实现client-server例子
 * 用两个FIFO并用fork生成一个子进程，客户作为父进程运行，服务器则作为子进程运行
 *                    parent process               child process
 *                    --------------               -------------
 * pathname--stdin--> |            | --pathname--> |           |       -------
 *                    |  client    |               |  server   | <---- | file |
 * context<--stdout-- |            | <--context--  |           |       -------
 *                    --------------               -------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAXSIZE 256
#define FIFO1 "/tmp/fifo.1"
#define FIFO2 "/tmp/fifo.2"
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

void client(int readfd, int writefd);
void server(int readfd, int writefd);
void error_handler(char *msg);

int main(int argc, char **argv) {
  int readfd, writefd;
  pid_t pid;
  if((mkfifo(FIFO1, FILE_MODE) == -1) && (errno != EEXIST))
	error_handler("mkfifo(FIFO1) error");
  if((mkfifo(FIFO2, FILE_MODE) == -1) && (errno != EEXIST)) {
	unlink(FIFO1); // delete the file
	error_handler("mkfifo(FIFO2) error");
  }

  if((pid = fork()) == -1)
	error_handler("fork() error");
  if(pid == 0) { // child process: server
	if((readfd = open(FIFO1, O_RDONLY)) == -1)
	  error_handler("child open(FIFO1, R) error");
	if((writefd = open(FIFO2, O_WRONLY)) == -1)
	  error_handler("chidl open(FIFO2, W) error");
	server(readfd, writefd);
	return 0;
  }
  if((writefd = open(FIFO1, O_WRONLY)) == -1)
	error_handler("parent open(FIFO1, W) error");
  if((readfd = open(FIFO2, O_RDONLY)) == -1)
	error_handler("parent open(FIFO2, R) error");
  client(readfd, writefd);
  wait(NULL);
  close(readfd);
  close(writefd);
  unlink(FIFO1);
  unlink(FIFO2);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}

void client(int readfd, int writefd) {
  ssize_t n;
  char buf[MAXSIZE];
  fputs("input pathname: ", stdout);
  fgets(buf, MAXSIZE, stdin);
  write(writefd, buf, strlen(buf)-1); // delete '\n' character

  while((n = read(readfd, buf, MAXSIZE)) > 0) {
	buf[n] = 0;
	fputs(buf, stdout);
  }
}

void server(int readfd, int writefd) {
  int fd;
  ssize_t n;
  char buf[MAXSIZE+1];
  if((n = read(readfd, buf, MAXSIZE)) == 0)
	error_handler("end-of-file while reading pathname");
  buf[n] = 0;

  if((fd = open(buf, O_RDONLY)) == -1) {
	snprintf(buf+n, sizeof(buf)-n, ": can't open, %s\n", strerror(errno));
	write(writefd, buf, strlen(buf));
  } else {
	snprintf(buf, sizeof(buf), "context of file: \n");
	write(writefd, buf, strlen(buf));

	while((n = read(fd, buf, MAXSIZE)) > 0) 
	  write(writefd, buf, strlen(buf));
	write(writefd, "\n", 1);
	close(fd);
  } 
}
