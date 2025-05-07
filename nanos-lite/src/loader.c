#include "common.h"
#include "memory.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern size_t get_ramdisk_size();
extern ssize_t fs_read(int fd, void *buf, size_t len);
extern size_t fs_filesz(int fd);
extern int fs_open(const char *pathname, int flags, int mode);
extern int fs_close(int fd);

uintptr_t loader(_Protect *as, const char *filename)
{
  // pa3
  // int fd = fs_open(filename, 0, 0);
  // Log("fd = %d\n",fd);
  // Log("filename = %s\n",filename);
  // fs_read(fd, DEFAULT_ENTRY, fs_filesz(fd));
  // fs_close(fd);

  int fd = fs_open(filename, 0, 0);
  const size_t file_size = fs_filesz(fd);
  const size_t total_pages = file_size / PGSIZE + (file_size % PGSIZE != 0);

  for (size_t i = 0; i < total_pages; ++i)
  {
    void *pa = new_page();
    _map(as, DEFAULT_ENTRY + i * PGSIZE, pa);
    const size_t read_size = (i == total_pages - 1)
                                 ? file_size - i * PGSIZE // 最后一页
                                 : PGSIZE;                // 完整页
    fs_read(fd, pa, read_size);
  }
  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}
