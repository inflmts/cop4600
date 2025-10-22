#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
  char buf[1024];
  ssize_t len;
  int fd;
  int n = 0;

#ifdef PART1
  fd = 0;
#else
  fd = open("part2.fifo", O_RDONLY);
  if (fd == -1) {
    perror("failed to open part2.fifo");
    exit(1);
  }
#endif

  while (1) {
    len = read(fd, buf, sizeof(buf));
    if (len < 0) {
      perror("read");
      exit(1);
    }
    if (len == 0) {
      break;
    }
    while (len) {
      if (buf[--len] == '\n') {
        ++n;
      }
    }
  }
  printf("Program failed on operation %d.\n", n);
  return 0;
}
