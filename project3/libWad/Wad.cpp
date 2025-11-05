#include <iostream>
#include <memory>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "Wad.h"

static bool is_start(const char *name)
{
  size_t namelen = strnlen(name, sizeof(name));
  return namelen >= 6 && !memcmp(name + namelen - 6, "_START", 6);
}

static bool is_map(const char *name)
{
  return name[0] == 'E'
      && name[1] >= '0' && name[1] <= '9'
      && name[2] == 'M'
      && name[3] >= '0' && name[3] <= '9'
      && name[4] == 0;
}

static bool is_end(const char *name)
{
  size_t namelen = strnlen(name, sizeof(name));
  return namelen >= 4 && !memcmp(name + namelen - 4, "_END", 4);
}

static bool is_file(const WadDescriptor *desc)
{
  return desc && !(desc == WAD_ROOT || is_start(desc->name) || is_map(desc->name));
}

static bool is_dir(const WadDescriptor *desc)
{
  return desc && (desc == WAD_ROOT || is_start(desc->name) || is_map(desc->name));
}

void Wad::skip(WadDescriptor *& desc)
{
  int depth = 0;
  do {
    if (is_start(desc->name)) {
      ++depth;
    } else if (is_end(desc->name)) {
      --depth;
    } else if (is_map(desc->name)) {
      desc += 10;
    }
    ++desc;
  } while (depth);
}

Wad::~Wad()
{
  close(fd);
}

#define WAD_ITER(d, body) \
  if (d == WAD_ROOT) { \
    for (d = descriptors.data(); d != descriptors.data() + descriptors.size(); skip(d)) body; \
  } else if (is_start(d->name)) { \
    for (++d; !is_end(d->name); skip(d)) body; \
  } else if (is_map(d->name)) { \
    ++d; for (int i = 0; i < 10; ++i, ++d) body; \
  }

WadDescriptor *Wad::resolve(const std::string& path, char *create_name)
{
  const char *s = path.c_str();
  WadDescriptor *desc = WAD_ROOT;
  char component[8];
  char name[8];

  if (*s != '/') {
    return nullptr;
  }
  while (*(++s) == '/');

next:
  if (!*s) {
    return create_name ? nullptr : desc;
  }
  const char *begin = s;
  while (*s && *s != '/') {
    ++s;
  }
  if (s - begin > 8) {
    return nullptr;
  }
  memset(component, 0, sizeof(component));
  memcpy(component, begin, s - begin);
  while (*s == '/') {
    ++s;
  }
  WAD_ITER(desc, {
    memcpy(name, desc->name, sizeof(name));
    if (is_start(desc->name)) {
      memset(name + strnlen(name, sizeof(name)) - 6, 0, 6);
    }
    if (!memcmp(component, name, sizeof(name))) {
      goto next;
    }
  }) else {
    return nullptr;
  }
  if (create_name && !*s && (
      desc == descriptors.data() + descriptors.size() || is_end(desc->name))) {
    memcpy(create_name, component, 8);
    return desc;
  }
  return nullptr;
}

Wad *Wad::loadWad(const std::string& path)
{
  int fd = open(path.c_str(), O_RDWR);
  if (fd < 0) {
    return nullptr;
  }

  WadHeader header;
  pread(fd, &header, sizeof(WadHeader), 0);
  std::unique_ptr<Wad> wad(new Wad());
  wad->fd = fd;
  memcpy(wad->magic, header.magic, sizeof(header.magic));
  wad->doffset = header.doffset;
  wad->descriptors.resize(header.dcount);
  pread(fd, wad->descriptors.data(), header.dcount * sizeof(WadDescriptor), header.doffset);

  return wad.release();
}

std::string Wad::getMagic()
{
  return std::string(magic, 4);
}

bool Wad::isContent(const std::string& path)
{
  return is_file(resolve(path));
}

bool Wad::isDirectory(const std::string& path)
{
  return is_dir(resolve(path));
}

int Wad::getSize(const std::string& path)
{
  WadDescriptor *desc = resolve(path);
  return is_file(desc) ? desc->size : -1;
}

int Wad::getContents(const std::string& path, char *buffer, int length, int offset)
{
  WadDescriptor *desc = resolve(path);
  if (!is_file(desc)) {
    return -1;
  }
  if (offset >= (int)desc->size) {
    return 0;
  }
  if (length > (int)desc->size - offset) {
    length = (int)desc->size - offset;
  }
  return pread(fd, buffer, length, desc->offset + offset);
}

int Wad::getDirectory(const std::string& path, std::vector<std::string> *directory)
{
  WadDescriptor *desc = resolve(path);
  if (!desc) {
    return -1;
  }
  int count = 0;
  WAD_ITER(desc, {
    size_t namelen = strnlen(desc->name, sizeof(desc->name));
    if (is_start(desc->name)) {
      namelen -= 6;
    }
    directory->emplace_back(desc->name, namelen);
    ++count;
  }) else {
    return -1;
  }
  return count;
}

void Wad::createDirectory(const std::string& path)
{
  size_t namelen;
  WadDescriptor start {};
  WadDescriptor *desc = resolve(path, start.name);
  if (!desc || (namelen = strnlen(start.name, sizeof(start.name))) > 2) {
    return;
  }
  WadDescriptor end = start;
  memcpy(start.name + namelen, "_START", 6);
  memcpy(end.name + namelen, "_END", 4);
  uint32_t index = desc - descriptors.data();
  descriptors.insert(descriptors.begin() + index, { start, end });
  uint32_t dcount = descriptors.size();
  pwrite(fd, &dcount, 4, 4);
  pwrite(fd, descriptors.data() + index, (dcount - index) * sizeof(WadDescriptor), doffset + index * sizeof(WadDescriptor));
}

void Wad::createFile(const std::string& path)
{
  WadDescriptor file {};
  WadDescriptor *desc = resolve(path, file.name);
  if (!desc || is_start(file.name) || is_map(file.name) || is_end(file.name)) {
    return;
  }
  uint32_t index = desc - descriptors.data();
  descriptors.insert(descriptors.begin() + index, file);
  uint32_t dcount = descriptors.size();
  pwrite(fd, &dcount, 4, 4);
  pwrite(fd, descriptors.data() + index, (dcount - index) * sizeof(WadDescriptor), doffset + index * sizeof(WadDescriptor));
}

int Wad::writeToFile(const std::string& path, const char *buffer, int length, int offset)
{
  WadDescriptor *desc = resolve(path);
  if (!is_file(desc)) {
    return -1;
  }
  if ((uint32_t)offset != desc->size) {
    return 0;
  } else if (!desc->size) {
    desc->offset = doffset;
  } else if (desc->offset + desc->size != doffset) {
    return 0;
  }
  doffset += length;
  desc->size += length;
  pwrite(fd, &doffset, 4, 8);
  pwrite(fd, buffer, length, doffset - length);
  pwrite(fd, descriptors.data(), descriptors.size() * sizeof(WadDescriptor), doffset);
  return length;
}
