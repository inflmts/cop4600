#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct pipes
{
  int r01, w01, r10, w10, r12, w12, r20, w20;
};

static void sorter(struct pipes *fds)
{
  int nums[5];
  read(fds->r01, nums, sizeof(nums));

  /* insertion sort */
  for (int i = 0; i < 4; ++i) {
    int m = i;
    for (int j = i + 1; j < 5; ++j) {
      if (nums[j] < nums[m]) {
        m = j;
      }
    }
    if (m != i) {
      int temp = nums[m];
      nums[m] = nums[i];
      nums[i] = temp;
    }
  }

  write(fds->w10, nums, sizeof(nums));
  write(fds->w12, nums, sizeof(nums));
}

static void finder(struct pipes *fds)
{
  int nums[5];
  read(fds->r12, nums, sizeof(nums));
  write(fds->w20, &nums[2], sizeof(int));
}

int main(int argc, char **argv)
{
  int nums[5];
  struct pipes fds;

  if (argc != 6) {
    fputs("Please provide 5 integers as arguments.\n", stderr);
    exit(2);
  }

  for (int i = 0; i < 5; ++i) {
    nums[i] = atoi(argv[i + 1]);
  }

  for (int i = 0; i < 8; i += 2) {
    pipe(reinterpret_cast<int *>(&fds) + i);
  }

  if (fork()) {
    sorter(&fds);
    _exit(0);
  }

  if (fork()) {
    finder(&fds);
    _exit(0);
  }

  write(fds.w01, nums, sizeof(nums));
  read(fds.r10, nums, sizeof(nums));
  printf("Sorted list of ints: %d  %d  %d  %d  %d\n",
      nums[0], nums[1], nums[2], nums[3], nums[4]);
  read(fds.r20, nums, sizeof(int));
  printf("Median: %d\n", nums[0]);
}
