/*
 * 多线程并发客户器端的实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_hanlder(char *msg);
char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char **argv) {
  int sockfd;
  struct sockaddr_in serv_addr;
  pthread_t send_thread, recv_thread;
  void *thread_return;
  if(argc != 4) {
	printf("usage: %s <ip> <port> <name>\n", argv[0]);
	exit(1);
  }

  sprintf(name, "[%s]", argv[3]);
  if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	error_hanlder("socket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_addr.sin_port = htons(atoi(argv[2]));
  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	error_hanlder("connect() error");

  pthread_create(&send_thread, NULL, send_msg, (void*)&sockfd);
  pthread_create(&recv_thread, NULL, recv_msg, (void*)&sockfd);
  pthread_join(send_thread, &thread_return);
  pthread_join(recv_thread, &thread_return);
  close(sockfd);
  return 0;
}

void *send_msg(void *arg) {
  int sockfd = *((int*)arg);
  char name_msg[NAME_SIZE+BUF_SIZE];
  while(1) {
	fgets(msg, BUF_SIZE, stdin);
	if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) {
	  close(sockfd);
	  exit(0);
	}
	sprintf(name_msg, "%s %s", name, msg);
	write(sockfd, name_msg, strlen(name_msg));
  }
  return NULL;
}

void *recv_msg(void *arg) {
  int sockfd = *((int*)arg);
  char name_msg[NAME_SIZE+BUF_SIZE];
  int str_len;
  while(1) {
	if((str_len = read(sockfd, name_msg, NAME_SIZE+BUF_SIZE-1)) == -1)
	  return (void*)-1;
	name_msg[str_len] = 0;
	fputs(name_msg, stdout);
  }
  return NULL;
}

void error_hanlder(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
