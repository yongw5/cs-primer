#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define SEQFILE "seqno"
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MAXSIZE 256
#define error_hanlder(msg) \
  do { \
	printf("%s:%d %s, %s\n", __FILE__, __LINE__, #msg, strerror(errno)); \
	exit(1); \
  }while(0)
void my_lock(int), my_unlock(int);

int main(int argc, char **argv) {
  int fd;
  long i, seqno;
  pid_t	pid;
  ssize_t n;
  char buf[MAXSIZE];

  pid = getpid();
  if((fd = open(SEQFILE, O_RDWR , FILE_MODE)) == -1)
	error_hanlder(open error);
  
  for(i = 0; i < 20; ++i) {
	my_lock(fd); /* lock the file */
	lseek(fd, 0L, SEEK_SET); /* rewind before read */
	if((n = read(fd, buf, MAXSIZE)) == -1)
	  error_hanlder(read error);
	buf[n] = 0; /* null terminate for sscanf */
	n = sscanf(buf, "%ld\n", &seqno);
	printf("%s: pid = %ld, seq# = %ld\n", argv[0], (long)pid, seqno);
	seqno++; /* incrment sequence number */

	snprintf(buf, sizeof(buf), "%ld\n", seqno);
	lseek(fd, 0L, SEEK_SET); /* rewind before write */
	if(write(fd, buf, strlen(buf)) == -1)
	  error_hanlder(write error);

	my_unlock(fd); /* unlock the file */
  }
  return 0;
}

void my_lock(int fd) {
  struct flock lock;
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;
  lock.l_start = 0; /* write lock entire file */

  fcntl(fd, F_SETLKW, &lock);
}

void my_unlock(int fd) {
  struct flock lock;
  lock.l_type = F_UNLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0; /* unlock entire file */

  fcntl(fd, F_SETLK, &lock);
}
