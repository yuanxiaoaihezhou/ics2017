#include "common.h"
#include "syscall.h"
#include "fs.h"

extern int fs_open(const char *pathname, int flags, int mode);
extern ssize_t fs_read(int fd, void *buf, size_t len);
extern ssize_t fs_write(int fd, const void *buf, size_t len);
extern off_t fs_lseek(int fd, off_t offset, int whence);
extern int fs_close(int fd);

_RegSet *do_syscall(_RegSet *r)
{
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r);

  switch (a[0])
  {
  case SYS_none:
    SYSCALL_ARG1(r) = 1;
    break;
  case SYS_exit:
    _halt(SYSCALL_ARG2(r));
    break;
  case SYS_write:
    {
    int fd = (int)SYSCALL_ARG2(r);
    char *buf = (char *)SYSCALL_ARG3(r);
    int len = (int)SYSCALL_ARG4(r);
    SYSCALL_ARG1(r) = fs_write(fd, buf, len);
    break;
    }
  case SYS_brk:
    SYSCALL_ARG1(r) = 0;
    break;
  case SYS_open:
    {
    const char *pathname = (const char *)SYSCALL_ARG2(r);
    int flags = (int)SYSCALL_ARG3(r);
    int mode = (int)SYSCALL_ARG4(r);
    SYSCALL_ARG1(r) = fs_open(pathname, flags, mode);
    break;
    }
  case SYS_read:
    {
    int fd = (int)SYSCALL_ARG2(r);
    char *buf = (char *)SYSCALL_ARG3(r);
    int len = (int)SYSCALL_ARG4(r);
    SYSCALL_ARG1(r) = fs_read(fd, buf, len);
    break;
    }
  case SYS_close:
    {
    int fd = (int)SYSCALL_ARG2(r);
    SYSCALL_ARG1(r) = fs_close(fd);
    break;
    } 
  case SYS_lseek:
    {
    int fd = (int)SYSCALL_ARG2(r);
    off_t offset = (off_t)SYSCALL_ARG3(r);
    int whence = (int)SYSCALL_ARG4(r);
    SYSCALL_ARG1(r) = fs_lseek(fd, offset, whence);
    break;
    }
  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}
