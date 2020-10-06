/**
 * 点分十进制格式 ==> in_addr
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
 if(argc != 2) {
  printf("usage: %s <IP>\n", argv[0]);
  exit(1);
 }
  struct sockaddr_in net_addr;
  if(!inet_aton(argv[1], &net_addr.sin_addr)) {
    fputs("conversion error\n", stderr);
	exit(1);
  } else
    printf("ip: %15s\tnetwork ordered interger add: %#x\n", argv[1], net_addr.sin_addr.s_addr);
  return 0;
}
