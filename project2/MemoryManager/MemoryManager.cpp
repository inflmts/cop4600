#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "MemoryManager.h"

/*
 * "Up to you though! We will not be testing you on your design skills,
 *  just whether or not it functions correctly when you submit it."
 *
 * So... I used a bytemap.
 *
 * Each word in the pool corresponds to a byte in the map.
 * This has the obvious drawback that the map can be very large.
 */

#define MEM_NONE    0
#define MEM_BLOCK   1
#define MEM_HOLE    2

MemoryManager::MemoryManager(unsigned int wordSize, MemoryAllocator allocator) :
  word_size(wordSize),
  allocator(allocator),
  pool_size_words(0),
  pool(nullptr)
{}

MemoryManager::~MemoryManager()
{
  shutdown();
}

void MemoryManager::initialize(size_t sizeInWords)
{
  /* Challenge accepted. */
  shutdown();
  pool_size_words = sizeInWords;
  total_size = pool_size_words * word_size + pool_size_words;
  num_holes = 1;
  pool = static_cast<unsigned char *>(mmap(nullptr, total_size,
      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
  if (pool == MAP_FAILED) {
    fprintf(stderr, "fatal: mmap failed: %s\n", strerror(errno));
    exit(-1);
  }
  map = pool + pool_size_words * word_size;
  map_end = map + pool_size_words;
  /* It starts off as one big hole */
  *map = MEM_HOLE;
}

void MemoryManager::shutdown()
{
  if (pool) {
    munmap(static_cast<void *>(pool), total_size);
    pool_size_words = 0;
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
  int offset = allocator(size_words, static_cast<void *>(list));
  delete[] list;
  if (offset < 0) {
    return nullptr;
  }
  unsigned char *cur = map + offset;
  unsigned char *next = cur + size_words;
  assert(*cur == MEM_HOLE);
  *cur = MEM_BLOCK;
  if (next < map_end && *next == MEM_NONE) {
    *next = MEM_HOLE;
  } else {
    --num_holes;
  }
  return static_cast<void *>(pool + offset * word_size);
}

void MemoryManager::free(void *address)
{
  if (!address) {
    return;
  }
  unsigned int offset = (static_cast<unsigned char *>(address) - pool) / word_size;
  unsigned char *cur, *prev;
  cur = map + offset;
  ++num_holes;
  *cur = MEM_HOLE;
  if (cur > map) {
    prev = cur;
    do {
      --prev;
    } while (*prev == MEM_NONE);
    if (*prev == MEM_HOLE) {
      --num_holes;
      *cur = MEM_NONE;
    }
  }
  do {
    ++cur;
    if (cur >= map_end) {
      return;
    }
  } while (*cur == MEM_NONE);
  if (*cur == MEM_HOLE) {
    --num_holes;
    *cur = MEM_NONE;
  }
}

void MemoryManager::setAllocator(MemoryAllocator allocator)
{
  this->allocator = allocator;
}

int MemoryManager::dumpMemoryMap(char *filename)
{
  uint16_t *list = static_cast<uint16_t *>(getList());
  int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0) {
    delete[] list;
    return -1;
  }
  uint16_t *arr = list;
  unsigned int count = *arr;
  char buf[32];
  while (count) {
    unsigned int start = *(++arr);
    unsigned int len = *(++arr);
    int chars = snprintf(buf, sizeof(buf), "[%u, %u]", start, len);
    if (--count) {
      memcpy(buf + chars, " - ", 4);
      chars += 3;
    }
    write(fd, buf, chars);
  }
  close(fd);
  delete[] list;
  return 0;
}

void *MemoryManager::getList()
{
  if (!pool) {
    return nullptr;
  }
  uint16_t *list = new uint16_t[1 + num_holes * 2];
  uint16_t *value = list;
  unsigned int begin = UINT_MAX;
  *value = num_holes;
  for (unsigned int offset = 0; offset < pool_size_words; ++offset) {
    if (map[offset] == MEM_HOLE) {
      *(++value) = offset;
      begin = offset;
    } else if (begin != UINT_MAX && map[offset] == MEM_BLOCK) {
      *(++value) = offset - begin;
      begin = UINT_MAX;
    }
  }
  if (begin != UINT_MAX) {
    *(++value) = pool_size_words - begin;
  }
  return list;
}

void *MemoryManager::getBitmap()
{
  uint16_t len = (pool_size_words + 7) >> 3;
  unsigned char *bitmap, *cur;
  cur = bitmap = new unsigned char[2 + len];
  *(cur++) = len & 0xff;
  *(cur++) = len >> 8;
  memset(cur, 0, len);
  int block = 0;
  for (unsigned int offset = 0; offset < pool_size_words; ++offset) {
    if (map[offset] == MEM_BLOCK) {
      block = 1;
    } else if (map[offset] == MEM_HOLE) {
      block = 0;
    }
    if (block) {
      cur[offset >> 3] |= 1 << (offset & 7);
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
  return pool_size_words * word_size;
}

int bestFit(int sizeInWords, void *list)
{
  uint16_t *arr = static_cast<uint16_t *>(list);
  unsigned int min_len = UINT_MAX;
  int min_offset = -1;
  unsigned int count = *arr;
  while (count) {
    uint16_t offset = *(++arr);
    uint16_t len = *(++arr);
    if (len < min_len) {
      min_len = len;
      min_offset = offset;
    }
    --count;
  }
  return min_offset;
}

int worstFit(int sizeInWords, void *list)
{
  uint16_t *arr = static_cast<uint16_t *>(list);
  unsigned int max_len = 0;
  int max_offset = -1;
  unsigned int count = *arr;
  while (count) {
    uint16_t offset = *(++arr);
    uint16_t len = *(++arr);
    if (len > max_len) {
      max_len = len;
      max_offset = offset;
    }
    --count;
  }
  return max_offset;
}
