#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <sys/random.h>
#include <unistd.h>

void thread_function(unsigned int id, unsigned int target)
{
  char buf[64];
  int buflen;
  unsigned int value;
  do {
    getrandom(&value, sizeof(value), 0);
    value = value % 10000;
  } while (value != target);
  buflen = snprintf(buf, sizeof(buf), "Thread %u completed.\n", id);
  // Use write() to prevent interleaved output.
  // There might be a better way, but I'm lazy.
  write(1, buf, buflen);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <target>\n", argv[0]);
    return 2;
  }

  unsigned int target = strtoul(argv[1], NULL, 0);
  std::thread threads[10];

  if (target > 9999) {
    target = 9999;
  }
  for (unsigned int i = 0; i < 10; i++) {
    threads[i] = std::thread(thread_function, i, target);
  }
  for (unsigned int i = 0; i < 10; i++) {
    threads[i].join();
  }

  printf("All threads have finished finding numbers.\n");
  return 0;
}
