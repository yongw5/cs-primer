/*
 * 实现多播Sender和Receiver
 * Sender：向×××组多播播文件中保存的新闻信息
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define TTL 64
#define BUF_SIZE 64
void error_handler(char *msg);

int main(int argc, char **argv) {
  int send_sockfd;
  struct sockaddr_in mul_addr;
  int time_live = TTL;
  FILE *fp;
  char buf[BUF_SIZE];

  if(argc != 3) {
	printf("usage: %s <GroupIp> <PORT>\n", argv[0]);
	exit(1);
  }

  if((send_sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
	error_handler("socket() error");
  memset(&mul_addr, 0, sizeof(mul_addr));
  mul_addr.sin_family = AF_INET;
  mul_addr.sin_addr.s_addr = inet_addr(argv[1]); // Multicast IP
  mul_addr.sin_port = htons(atoi(argv[2]));      // Multicast Port
  if(setsockopt(send_sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &time_live, sizeof(time_live)) == -1)
	error_handler("setsockopt() error");
  if((fp = fopen("news.txt", "r")) == NULL)
	error_handler("fopen() error");
  while(!feof(fp)) {
	fgets(buf, BUF_SIZE, fp);
	sendto(send_sockfd, buf, strlen(buf), 0, (struct sockaddr*)&mul_addr, sizeof(mul_addr));
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
