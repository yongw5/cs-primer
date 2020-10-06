/**
 * 点分十进制格式 ==> 32位整型
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
  if(argc != 2) {
	printf("usage: %s <IP>", argv[0]);
	exit(1);
  }
  unsigned long conv_addr = inet_addr(argv[1]);
  if(conv_addr == INADDR_NONE)
    printf("error occured!\n");
  else
    printf("ip: %15s\tnetwork order integet addr: %#lx\n", argv[1], conv_addr);
  return 0;
}
