/*
 * 实现广播Sender和Receiver
 * Sender：同一网络中的主机广播文件中保存的新闻信息
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 64
void error_handler(char *msg);

int main(int argc, char **argv) {
  int send_sockfd;
  struct sockaddr_in brd_addr;
  int so_brd = 1;
  FILE *fp;
  char buf[BUF_SIZE];

  if(argc != 3) {
	printf("usage: %s<BroadcaseIP> <PORT>\n", argv[0]);
	exit(1);
  }

  if((send_sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
	error_handler("socket() error");
  memset(&brd_addr, 0, sizeof(brd_addr));
  brd_addr.sin_family = AF_INET;
  brd_addr.sin_addr.s_addr = inet_addr(argv[1]); // Multicast IP
  brd_addr.sin_port = htons(atoi(argv[2]));      // Multicast Port
  if(setsockopt(send_sockfd, SOL_SOCKET, SO_BROADCAST, &so_brd, sizeof(so_brd)) == -1)
	error_handler("setsockopt() error");
  if((fp = fopen("news.txt", "r")) == NULL)
	error_handler("fopen() error");
  while(!feof(fp)) {
	fgets(buf, BUF_SIZE, fp);
	sendto(send_sockfd, buf, strlen(buf), 0, (struct sockaddr*)&brd_addr, sizeof(brd_addr));
	sleep(2);
  }
  fclose(fp);
  close(send_sockfd);
  return 0;
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
