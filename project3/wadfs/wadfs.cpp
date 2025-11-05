#include <iostream>
#include <memory>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fuse/fuse.h>

#include "Wad.h"

std::unique_ptr<Wad> wad;

int wadfs_getattr(const char *path, struct stat *st)
{
  if (wad->isContent(path)) {
    st->st_mode = S_IFREG | 0777;
    st->st_size = wad->getSize(path);
    return 0;
  } else if (wad->isDirectory(path)) {
    st->st_mode = S_IFDIR | 0777;
    return 0;
  } else {
    return -ENOENT;
  }
}

int wadfs_mknod(const char *path, mode_t mode, dev_t dev)
{
  wad->createFile(path);
  return 0;
}

int wadfs_mkdir(const char *path, mode_t mode)
{
  wad->createDirectory(path);
  return 0;
}

int wadfs_read(const char *path, char *buf, size_t len, off_t offset, struct fuse_file_info *fi)
{
  int res = wad->getContents(path, buf, len, offset);
  return res < 0 ? -EPERM : res;
}

int wadfs_write(const char *path, const char *buf, size_t len, off_t offset, struct fuse_file_info *fi)
{
  return wad->writeToFile(path, buf, len, offset) == (int)len ? len : -EPERM;
}

int wadfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
  std::vector<std::string> entries;
  if (wad->getDirectory(path, &entries) == -1) {
    return -ENOTDIR;
  }
  filler(buf, ".", nullptr, 0);
  filler(buf, "..", nullptr, 0);
  for (std::string& entry : entries) {
    filler(buf, entry.c_str(), nullptr, 0);
  }
  return 0;
}

struct fuse_operations wadfs_ops = {
  .getattr = wadfs_getattr,
  .mknod = wadfs_mknod,
  .mkdir = wadfs_mkdir,
  .read = wadfs_read,
  .write = wadfs_write,
  .readdir = wadfs_readdir
};

static void usage()
{
  std::cerr << "usage: wadfs [-fs] source mountpoint\n";
}

static const char *debug_argv[] = { "", "-d" };
static struct fuse_args debug_args = FUSE_ARGS_INIT(2, const_cast<char **>(debug_argv));

/* See lib/helper.c in FUSE source. */
int main(int argc, char **argv)
{
  bool debug = false;
  bool foreground = false;
  int opt;

  while ((opt = getopt(argc, argv, "dfhs")) != -1) {
    switch (opt) {
      case 'd':
        debug = true;
        foreground = true;
        break;
      case 'f':
        foreground = true;
        break;
      case 's':
        break;
      default:
        usage();
        return 2;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 2) {
    usage();
    return 2;
  }

  const char *source = argv[0];
  const char *mountpoint = argv[1];

  wad.reset(Wad::loadWad(source));

  int ret = 1;
  struct fuse_chan *ch;
  if (!(ch = fuse_mount(mountpoint, debug ? &debug_args : nullptr))) {
    return 1;
  }
  struct fuse *fuse = nullptr;
  if (!(fuse = fuse_new(ch, debug ? &debug_args : nullptr, &wadfs_ops, sizeof(wadfs_ops), nullptr))) {
    goto finish;
  }
  fuse_daemonize(foreground);
  if (fuse_set_signal_handlers(fuse_get_session(fuse))) {
    goto finish;
  }
  ret = fuse_loop(fuse);
  fuse_remove_signal_handlers(fuse_get_session(fuse));
finish:
  fuse_unmount(mountpoint, ch);
  if (fuse) {
    fuse_destroy(fuse);
  }
  return ret;
}
