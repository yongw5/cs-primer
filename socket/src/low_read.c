/**
 * read函数调用例子
 * 读取data.txt保存的数据
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SZ 100
void error_handler(char *msg);

int main(int argc, int argv) {
  int fd;
  char buf[BUF_SZ];
  if((fd = open("data.txt", O_RDONLY)) == -1)
    error_handler("open error");
  if(read(fd, buf, sizeof(buf)-1) == -1)
    error_handler("read error");
  printf("file data: %s\n", buf);
  close(fd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
