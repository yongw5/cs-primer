/*
 * 进程间通信
 * PIPE管道
 */

#include <stdio.h>
#include <unistd.h>
#define BUF_SIZE 30

int main(int argc, char *argv) {
  int fds[2];
  char msg[] = "who are you";
  char buf[BUF_SIZE];
  pid_t pid;
  pipe(fds);
  pid = fork();
  if(pid == 0) {
	printf("child process write message: %s\n", msg);
	write(fds[1], msg, sizeof(msg));
  } else {
	printf("parent process read message: ");
	read(fds[0], buf, BUF_SIZE);
	puts(buf);
  }
  return 0;
}
