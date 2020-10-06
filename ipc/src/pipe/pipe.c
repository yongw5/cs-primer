/**
 * 用管道实现client-server例子
 * 用两个管道并用fork生成一个子进程，客户作为父进程运行，服务器则作为子进程运行
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
#include <wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXSIZE 256
void client(int readfd, int writefd);
void server(int readfd, int writefd);
void error_handler(char *msg);
int main() {
  int serv2clnt[2], clnt2serv[2];
  pid_t pid;
  if(pipe(serv2clnt) == -1)
    error_handler("pipe() error");
  if(pipe(clnt2serv) == -1)
    error_handler(("pipe() error"));
  if ((pid = fork()) < 0)
	error_handler("fork() error");

  if(pid == 0) { // child process: server
    close(clnt2serv[1]);
    close(serv2clnt[0]);
    server(clnt2serv[0], serv2clnt[1]);
    return 0;
  }
  close(serv2clnt[1]);
  close(clnt2serv[0]);
  client(serv2clnt[0], clnt2serv[1]);
  wait(NULL);
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
  write(writefd, buf, strlen(buf)-1); // send pathname to server

  while((n = read(readfd, buf, MAXSIZE)) > 0) { // receive message from server
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
    char msg[] = "context of file: \n";
    write(writefd, msg, sizeof(msg));
    while((n = read(fd, buf, MAXSIZE)) > 0) {
      write(writefd, buf, n);
    }
	write(writefd, "\n", 1);
    close(fd);
  }
}
