#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
    [_KEY_NONE] = "NONE",
    _KEYS(NAME)};

bool current_game = 0;
extern off_t fs_lseek(int fd, off_t offset, int whence);
size_t events_read(void *buf, size_t len)
{
  int key = _read_key();
  bool down = false;
  // Log("key = %d\n", key);
  if (key & 0x8000)
  {
    key ^= 0x8000;
    down = true;
  }
  if (key == _KEY_NONE)
  {
    unsigned long t = _uptime();
    sprintf(buf, "t %d\n", t);
  }
  else
  {
    sprintf(buf, "%s %s\n", down ? "kd" : "ku", keyname[key]);
    if(key == 13 && down) {
      current_game = (current_game == 0 ? 1 : 0);
      fs_lseek(5,0,0);
    }
  }
  return strlen(buf);
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len)
{
  memcpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) 
{
    offset >>= 2;   
    len >>= 2;       
    
    int x = offset % _screen.width;
    int y = offset / _screen.width;
    const uint32_t *pixels = (const uint32_t *)buf;

    int len1 = (len <= _screen.width - x) ? len : _screen.width - x;
    if (len1 > 0) {
        _draw_rect(pixels, x, y, len1, 1);
        pixels += len1;
    }

    int remaining = len - len1;
    if (remaining <= 0) return;

    int full_rows = remaining / _screen.width;
    int len2 = full_rows * _screen.width;
    if (full_rows > 0) {
        _draw_rect(pixels, 0, y + 1, _screen.width, full_rows);
        pixels += len2;
    }

    int len3 = remaining - len2;
    if (len3 > 0) {
        _draw_rect(pixels, 0, y + 1 + full_rows, len3, 1);
    }
}

void init_device()
{
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  strcpy(dispinfo, "WIDTH:400\nHEIGHT:300");
}
