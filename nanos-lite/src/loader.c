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
  int bytes = fs_filesz(fd);
  int n = bytes / PGSIZE;
  int m = bytes % PGSIZE;
  int i;
  void *pa;

  for (i = 0; i < n; i++) {
    pa = new_page();
    _map(as, DEFAULT_ENTRY + i * PGSIZE, pa);
    fs_read(fd, pa, PGSIZE);
  }

  pa = new_page();
  _map(as, DEFAULT_ENTRY + i * PGSIZE, pa);
  fs_read(fd, pa, m);

  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
