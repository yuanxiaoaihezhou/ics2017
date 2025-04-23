#include "fs.h"

typedef struct {
  char *name;         // 文件名
  size_t size;        // 文件大小
  off_t disk_offset;  // 文件在ramdisk中的偏移
  off_t open_offset;  // 文件被打开之后的读写指针
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

size_t fs_filesz(int fd) 
{
  return file_table[fd].size;
}

int fs_open(const char *pathname, int flags, int mode)
{
  Log("Opening file: %s", pathname); 

    for (int i = 0; i < NR_FILES; i++) { 
        if (file_table[i].name == NULL || file_table[i].name[0] == '\0') {
            continue; 
        }

        if (strcmp(file_table[i].name, pathname) == 0) {
            Log("Successfully opened file: %s (fd=%d)", pathname, i);
            return i; 
        }
    }

    Log("File not found: %s", pathname); 
    return -1;
}
ssize_t fs_read(int fd, void *buf, size_t len);
ssize_t fs_write(int fd, const void *buf, size_t len);
off_t fs_lseek(int fd, off_t offset, int whence);
int fs_close(int fd);