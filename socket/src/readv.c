/*
 * 由多个缓冲分别接收，可以减少IO函数调用次数
 */

#include <stdio.h>
#include <sys/uio.h>

#define BUF_SIZE 100

int main(int argc, char **argv) {
  struct iovec iov[2];
  char buf1[BUF_SIZE] = {0, };
  char buf2[BUF_SIZE] = {0, };
  int str_len;

  iov[0].iov_base = buf1;
  iov[0].iov_len = 5;
  iov[1].iov_base = buf2;
  iov[1].iov_len = BUF_SIZE;

  str_len = readv(0, iov, 2);
  printf("read bytes: %d\n", str_len);
  printf("first message: %s\n", buf1);
  printf("second message: %s\n", buf2);
  return 0;
}
