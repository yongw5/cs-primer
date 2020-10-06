/*
 * 销毁僵尸进程 wait
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  int status;
  pid_t pid = fork();

  if(pid == 0) {
    return 3;
  } else {
    printf("child pid: %d\n", pid);
	pid = fork();
	if(pid == 0) {
	  exit(7);
	} else {
	  printf("chile pid: %d\n", pid);
	  wait(&status);
	  if(WIFEXITED(status))
	    printf("child send one: %d\n", WEXITSTATUS(status));

	  wait(&status);
	  if(WIFEXITED(status))
	    printf("chile send two: %d\n", WEXITSTATUS(status));
	  sleep(30);
	}
  }
  return 0;
}
