/**
 * open函数调用例子
 * 创建新文件并保存数据
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

void error_handler(char *msg);

int main(int argc, char *argv) {
  int fd;
  char buf[] = "Let's go!\n";

  if((fd = open("data.txt", O_CREAT | O_WRONLY | O_TRUNC)) == -1)
    error_handler("open error");
  printf("file descriptor: %d\n", fd);

  if(write(fd, buf, sizeof(buf)) == -1)
    error_handler("write error");
  close(fd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
