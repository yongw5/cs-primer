/**
 * htons htonl 调用
 * 小（da）端序 ==> 网络字节序
 */

#include <stdio.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
  unsigned short host_port = 0x1234;
  unsigned short net_port;
  unsigned long host_addr = 0x12345678;
  unsigned long net_addr;

  net_port = htons(host_port);
  net_addr = htonl(host_addr);

  printf("host ordered prot: %#x\n", host_port);
  printf("network ordered prot: %#x\n", net_port);
  printf("host ordered address: %#x\n", host_addr);
  printf("network ordered address: %#x\n", net_addr);
  return 0;
}
