/*
 * 利用IP地址获取域名相关信息
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

void error_handler(char *msg);

int main(int argc, char **argv) {
  int i;
  struct hostent *host;
  struct sockaddr_in addr;
  if(argc != 2) {
    printf("usage: %s <ip>\n", argv[0]);
	exit(1);
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(argv[1]);
  if((host = gethostbyaddr((char*)&addr.sin_addr, 4, addr.sin_family)) == NULL)
    error_handler("gethostbyaddr() error");

  printf("offical name: %s\n", host->h_name);

  for(i = 0; host->h_aliases[i]; ++i)
    printf("aliases %d: %s\n", i+1, host->h_aliases[i]);

  printf("address type: %s\n", (host->h_addrtype == AF_INET)?"AF_INET":"AF_INET6");

  for(i = 0; host->h_addr_list[i]; ++i)
    printf("ip addr %d: %s\n", i+1,
	    inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
