#include "common.h"
#include "syscall.h"

extern char _end;
extern ssize_t fs_read(int fd, void *buf, size_t len);
extern ssize_t fs_write(int fd, const void *buf, size_t len);
extern int fs_open(const char *pathname, int flags, int mode);
extern off_t fs_lseek(int fd, off_t offset, int whence);
extern int fs_close(int fd);

static inline _RegSet* sys_write(_RegSet *r)
{
  int fd = (int)SYSCALL_ARG2(r);
  char *buf = (char *)SYSCALL_ARG3(r);
  int len = (int)SYSCALL_ARG4(r);

  if (fd == 1 || fd == 2)
  {
    for (int i = 0; i < len; i++)
    {
      _putc(buf[i]);
    }
    SYSCALL_ARG1(r) = len;
  }

  return NULL;
}

_RegSet *do_syscall(_RegSet *r)
{
  uintptr_t a[4], result = -1;
  a[0] = SYSCALL_ARG1(r);
  a[1] = SYSCALL_ARG2(r);
  a[2] = SYSCALL_ARG3(r);
  a[3] = SYSCALL_ARG4(r);

  switch (a[0])
  {
  case SYS_none:
    result = 1;
    break;
  case SYS_exit:
    _halt(a[1]);
    break;
  case SYS_write:
    // sys_write(r);
    result = fs_write(a[1], (void *)a[2], a[3]);
    break;
  case SYS_read:
    result = fs_read(a[1], (void *)a[2], a[3]);
    break;
  case SYS_brk:
    result = 0;
    break;
  case SYS_open:
    result = fs_open((char *)a[1], a[2], a[3]);
    break;
  case SYS_close:
    result = fs_close(a[1]);
    break;
  case SYS_lseek:
    result = fs_lseek(a[1], a[2], a[3]);
    break;
  default:
    panic("Unhandled syscall ID = %d", a[0]);
  }

  SYSCALL_ARG1(r) = result;

  return NULL;
}