#include "common.h"
#include "fs.h"

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern size_t get_ramdisk_size(void);

uintptr_t loader(_Protect *as, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  Log("fd = %d\n",fd);
  size_t f_size = fs_filesz(fd);
  fs_read(fd, (void *)0x4000000, f_size);
  fs_close(fd);
  
  return (uintptr_t)(0x4000000);
}
