#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "MemoryManager.h"

/*
 * "Up to you though! We will not be testing you on your design skills,
 *  just whether or not it functions correctly when you submit it."
 *
 * So... I used a bitmap.
 *
 * Each word in the pool corresponds to two bits in the map.
 * The order within each byte is low to high, the same as getBitmap().
 * The meaning of the bits are as follows:
 *
 *   00   middle of hole/block
 *   01   start of hole
 *   10   start of block following hole
 *   11   start of block following block
 */

#define shamt(i) (((i) & 3) << 1)

MemoryManager::MemoryManager(unsigned int wordSize, MemoryAllocator allocator) :
  word_size(wordSize), allocator(allocator), pool(nullptr) {}

MemoryManager::~MemoryManager()
{
  shutdown();
}

void MemoryManager::initialize(size_t sizeInWords)
{
  shutdown();
  num_words = sizeInWords;
  pool_size = num_words * word_size;
  total_size = pool_size + (num_words >> 2) + 1;

  /* Don't use stdlib or new? Challenge accepted.
   * mmap is slowly becoming my favorite system call.
   * Bonus: The memory is automatically initialized to zero. */
  pool = static_cast<unsigned char *>(mmap(nullptr, total_size,
      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  map = pool + pool_size;
  map[0] = 1;
  map[num_words >> 2] |= 2 << shamt(num_words);
}

void MemoryManager::shutdown()
{
  if (pool) {
    munmap(pool, total_size);
    pool = nullptr;
  }
}

void *MemoryManager::allocate(size_t size)
{
  if (!pool || !size) {
    return nullptr;
  }
  size_t size_words = (size - 1) / word_size + 1; /* ceil(size / word_size) */
  uint16_t *list = static_cast<uint16_t *>(getList());
  int i = allocator(size_words, list);
  delete[] list;
  if (i < 0) {
    return nullptr;
  }
  int j = i + size_words;
  map[i >> 2] |= 2 << shamt(i);
  map[j >> 2] |= 1 << shamt(j);
  return pool + i * word_size;
}

void MemoryManager::free(void *address)
{
  if (!pool || !address) {
    return;
  }
  int i = (static_cast<unsigned char *>(address) - pool) / word_size;
  map[i >> 2] &= ~(2 << shamt(i));
  do ++i;
  while (!(map[i >> 2] & 1 << shamt(i)));
  map[i >> 2] &= ~(1 << shamt(i));
}

void MemoryManager::setAllocator(MemoryAllocator allocator)
{
  this->allocator = allocator;
}

int MemoryManager::dumpMemoryMap(char *filename)
{
  if (!pool) {
    return -1;
  }
  int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0) {
    return -1;
  }
  uint16_t *list = static_cast<uint16_t *>(getList());
  uint16_t *p = list;
  int count = *p;
  char buf[32];
  while (count) {
    int start = *(++p);
    int len = *(++p);
    int chars = sprintf(buf, --count ? "[%u, %u] - " : "[%u, %u]", start, len);
    write(fd, buf, chars);
  }
  delete[] list;
  close(fd);
  return 0;
}

void *MemoryManager::getList()
{
  if (!pool) {
    return nullptr;
  }
  int num_holes = 0;
  for (int i = 0; i < num_words; ++i) {
    if ((map[i >> 2] >> shamt(i) & 3) == 1) {
      ++num_holes;
    }
  }
  uint16_t *list, *p;
  list = p = new uint16_t[1 + num_holes * 2];
  *p = num_holes;
  for (int begin, i = 0; i <= num_words; ++i) {
    unsigned char type = map[i >> 2] >> shamt(i) & 3;
    if (type == 1) {
      *(++p) = begin = i;
    } else if (type == 2) {
      *(++p) = i - begin;
    }
  }
  return list;
}

void *MemoryManager::getBitmap()
{
  if (!pool) {
    return nullptr;
  }
  uint16_t len = (num_words + 7) >> 3;
  unsigned char *bitmap, *p;
  bitmap = p = new unsigned char[2 + len];
  *(p++) = len & 0xff;
  *(p++) = len >> 8;
  memset(p, 0, len);
  int in_block;
  for (int i = 0; i < num_words; ++i) {
    unsigned char type = map[i >> 2] >> shamt(i) & 3;
    if (type) {
      in_block = type != 1;
    }
    if (in_block) {
      p[i >> 3] |= 1 << (i & 7);
    }
  }
  return bitmap;
}

unsigned int MemoryManager::getWordSize()
{
  return word_size;
}

void *MemoryManager::getMemoryStart()
{
  return pool;
}

unsigned int MemoryManager::getMemoryLimit()
{
  return pool ? pool_size : 0;
}

int bestFit(int sizeInWords, void *list)
{
  uint16_t *p = static_cast<uint16_t *>(list);
  int min_len = INT_MAX;
  int min_offset = -1;
  int count = *p;
  while (count) {
    int offset = *(++p);
    int len = *(++p);
    if (len >= sizeInWords && len < min_len) {
      min_len = len;
      min_offset = offset;
    }
    --count;
  }
  return min_offset;
}

int worstFit(int sizeInWords, void *list)
{
  uint16_t *p = static_cast<uint16_t *>(list);
  int max_len = sizeInWords - 1;
  int max_offset = -1;
  int count = *p;
  while (count) {
    uint16_t offset = *(++p);
    uint16_t len = *(++p);
    if (len > max_len) {
      max_len = len;
      max_offset = offset;
    }
    --count;
  }
  return max_offset;
}
