/*
 * 多线程并发服务器端的实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void *clnt_handler(void *arg);
void send_msg(char *msg, int len, int self_sockfd);
void error_handler(char *msg);

int clnt_cnt = 0;
int clnt_sockfdvec[MAX_CLNT];
pthread_mutex_t mutex;

int main(int argc, char **argv) {
  int serv_sockfd, clnt_sockfd;
  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addrlen;
  pthread_t thread_id;

  if(argc != 2) {
	printf("usage: %s <port>\n", argv[0]);
	exit(1);
  }
  pthread_mutex_init(&mutex, NULL);

  if((serv_sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	error_handler("socket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));
  if(bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
	error_handler("bind() error");
  if(listen(serv_sockfd, 5) == -1)
	error_handler("listen() error");

  while(1) {
	clnt_addrlen = sizeof(clnt_addr);
	if((clnt_sockfd = accept(serv_sockfd, (struct sockaddr*)&clnt_addr, &clnt_addrlen)) == -1) {
	  puts("accept() error");
	  continue;
	}
	pthread_mutex_lock(&mutex);
	clnt_sockfdvec[clnt_cnt++] = clnt_sockfd;
	pthread_mutex_unlock(&mutex);

	pthread_create(&thread_id, NULL, clnt_handler, (void*)&clnt_sockfd);
	pthread_detach(thread_id);
	printf("connected client IP: %s\n", inet_ntoa(clnt_addr.sin_addr));
  }
  close(serv_sockfd);
  return 0;
}

void *clnt_handler(void *arg) {
  int clnt_sockfd = *((int*)arg);
  int str_len = 0, i;
  char msg[BUF_SIZE];

  while((str_len = read(clnt_sockfd, msg, BUF_SIZE)) != 0)
	send_msg(msg, str_len, clnt_sockfd);
  pthread_mutex_lock(&mutex);
  for(i = 0; i < clnt_cnt; ++i) { // remove disconnected client 
	if(clnt_sockfd == clnt_sockfdvec[i]) {
	  while(i++ < clnt_cnt-1)
		clnt_sockfdvec[i] = clnt_sockfdvec[i+1];
	  break;
	}
  }
  clnt_cnt--;
  pthread_mutex_unlock(&mutex);
  close(clnt_sockfd);
  return NULL;
}

void send_msg(char *msg, int len, int self_sockfd) { // send to all excepth itself
  int i;
  pthread_mutex_lock(&mutex);
  for(i = 0; i < clnt_cnt; ++i)
	if(clnt_sockfdvec[i] != self_sockfd)
	  write(clnt_sockfdvec[i], msg, len);
  pthread_mutex_unlock(&mutex);
}

void error_handler(char *msg) {
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}
