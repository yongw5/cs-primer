/*
 * getsockopt函数的调用方法
 * 查看SO_TYPE
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

void error_handler(char *msg);

int main(int argc, char **argv) {
  int tcp_sock, udp_sock;
  int sock_type;
  socklen_t optlen;
  int state;

  optlen = sizeof(sock_type);
  if((tcp_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    error_handler("tcp socket() error");
  if((udp_sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    error_handler("udp socket() error");
  printf("SOCK_STREAM: %d\n", SOCK_STREAM);
  printf("SOCK_DGRAM: %d\n", SOCK_DGRAM);

  if((state = getsockopt(tcp_sock, SOL_SOCKET, SO_TYPE, (void*)&sock_type, &optlen)) == -1)
    error_handler("tcp getsockopt() error");
  printf("Socket type one: %d\n", sock_type);

  if((state = getsockopt(udp_sock, SOL_SOCKET, SO_TYPE, (void*)&sock_type, &optlen)) == -1)
    error_handler("udp getsockopt() error");
  printf("Socket type two: %d\n", sock_type);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
