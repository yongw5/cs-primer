#include <stdio.h>
#include <fcntl.h>

int main(void) {
  FILE *fp;
  int fd = open("data.dat", O_WRONLY|O_CREAT|O_TRUNC);
  if(fd == -1) {
	fputs("file open error", stdout);
	return -1;
  }
  printf("first file descriptor: %d\n", fd);
  fp = fdopen(fd, "w");
  fputs("tcp/ip socket programming\n", fp);
  printf("first file descriptor: %d\n", fileno(fd));
  fclose(fp);
  return 0;
}