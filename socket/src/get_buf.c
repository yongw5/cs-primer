/*
 * getsockopt函数的调用方法
 * 查看SO_SNDBUF & RCVBUF
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

void error_handler(char *msg);

int main(int argc, char **argv) {
  int sock;
  int snd_buf, rcv_buf;
  socklen_t len;

  if((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    error_handler("socket() error");
  len = sizeof(snd_buf);
  if(getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void*)&snd_buf, &len) == -1)
    error_handler("getsockopt() error");
  len = sizeof(rcv_buf);
  if(getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void*)&rcv_buf, &len) == -1)
    error_handler("getsockopt() error");

  printf("input buffer default size: %d\n", rcv_buf);
  printf("output buffer default size: %d\n", snd_buf);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
