#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <functional>
#include <stddef.h>
#include <stdint.h>

typedef std::function<int(int, void *)> MemoryAllocator;

class MemoryManager
{
  unsigned int word_size;
  MemoryAllocator allocator;
  unsigned int num_words;
  unsigned int pool_size;
  unsigned int total_size;
  unsigned char *pool;
  unsigned char *map;

public:
  MemoryManager(unsigned int wordSize, MemoryAllocator allocator);
  ~MemoryManager();
  void initialize(size_t sizeInWords);
  void shutdown();
  void *allocate(size_t sizeInBytes);
  void free(void *address);
  void setAllocator(MemoryAllocator allocator);
  int dumpMemoryMap(char *filename);
  void *getList();
  void *getBitmap();
  unsigned int getWordSize();
  void *getMemoryStart();
  unsigned int getMemoryLimit();
};

int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);

#endif
