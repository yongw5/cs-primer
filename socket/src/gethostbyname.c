/*
 * 利用域名获取IP地址
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

void error_handler(char *msg);

int main(int argc, char **argv) {
  int i;
  struct hostent *host;
  if(argc != 2) {
    printf("usage: %s <domain_name>", argv[0]);
	exit(1);
  }
  if((host = gethostbyname(argv[1])) == NULL)
    error_handler("gethostbyname() error");
  
  printf("official name: %s\n", host->h_name);
  for(i = 0; host->h_aliases[i]; ++i)
    printf("aliases %d: %s\n", i+1, host->h_aliases[i]);
  printf("address type: %s\n", (host->h_addrtype == AF_INET) ? "AF_INET" : "AF_INET6");
  for(i = 0; host->h_addr_list[i]; ++i)
    printf("ip address %d: %s\n", i+1, 
	  inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
