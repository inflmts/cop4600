#include <string>
#include <vector>

struct WadHeader
{
  char magic[4];
  uint32_t dcount;
  uint32_t doffset;
};

struct WadDescriptor
{
  uint32_t offset;
  uint32_t size;
  char name[8];
};

#define WAD_ROOT reinterpret_cast<WadDescriptor *>(-1)

class Wad
{
  int fd;
  char magic[4];
  uint32_t doffset;
  std::vector<WadDescriptor> descriptors;

  void skip(WadDescriptor *& desc);
  WadDescriptor *resolve(const std::string& path, char *create_name = nullptr);

public:
  ~Wad();
  static Wad *loadWad(const std::string& path);
  std::string getMagic();
  bool isContent(const std::string& path);
  bool isDirectory(const std::string& path);
  int getSize(const std::string& path);
  int getContents(const std::string& path, char *buffer, int length, int offset = 0);
  int getDirectory(const std::string& path, std::vector<std::string> *directory);
  void createDirectory(const std::string& path);
  void createFile(const std::string& path);
  int writeToFile(const std::string& path, const char *buffer, int length, int offset = 0);
};
