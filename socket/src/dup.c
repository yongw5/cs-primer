/*
 * 文件描述符复制
 */

#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int cfd1, cfd2;
  char str1[] = "Hi~ \n";
  char str2[] = "It's nice day~ \n";

  cfd1 = dup(1);
  cfd2 = dup2(cfd1, 7);

  printf("cfd1 = %d, cfd2 = %d\n", cfd1, cfd2);
  write(cfd1, str1, sizeof(str1));
  close(cfd1);
  printf("cfd1 is closed\n");

  write(cfd2, str2, sizeof(str2));
  close(cfd2);
  printf("cfd2 is closed\n");

  write(1, str1, sizeof(str1));
  printf("stdout is closed. and want to send str2\n");
  close(1);
  write(1, str2, sizeof(str2));
  return 0;
}
