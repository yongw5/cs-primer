#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#define BUF_SIZE 2048
void error_handler(char *msg);

int main(int argc, char **argv) {
  int sock;
  int snd_buf = BUF_SIZE, rcv_buf = BUF_SIZE;
  socklen_t len;

  if((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    error_handler("socket() error");
  if(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void*)&snd_buf, sizeof(snd_buf)) == -1)
    error_handler("setsockopt() error");
  len = sizeof(snd_buf);
  if(getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void*)&snd_buf, &len) == -1)
    error_handler("setsockopt error");
  
  if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void*)&rcv_buf, sizeof(rcv_buf)) == -1)
    error_handler("setsockopt() error");
  len = sizeof(rcv_buf);
  if(getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void*)&rcv_buf, &len) == -1)
    error_handler("getsockopt() error");

  printf("input buffer changed size: %d\n", rcv_buf);
  printf("output buffer changed size: %d\n", snd_buf);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
