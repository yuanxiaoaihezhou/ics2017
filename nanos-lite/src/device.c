#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
    [_KEY_NONE] = "NONE",
    _KEYS(NAME)};

size_t events_read(void *buf, size_t len)
{
  int key = _read_key();
  bool down = false;

  if (key & 0x8000)
  {
    key ^= 0x8000;
    down = true;
  }

  int written = 0;
  const char *log_action = down ? "down" : "up";

  if (key == _KEY_NONE)
  {
    unsigned long t = _uptime();
    written = snprintf((char *)buf, len, "t %lu\n", t);
  }
  else
  {
    const char *action = down ? "kd" : "ku";
    written = snprintf((char *)buf, len, "%s %s\n", action, keyname[key]);
    Log("Get key: %d %s %s\n", key, keyname[key], log_action);
  }

  if (written < 0)
  {
    return 0; 
  }
  else if ((size_t)written >= len)
  {
    return len - 1; 
  }
  else
  {
    return (size_t)written; 
  }
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len)
{
  memcpy(buf,dispinfo+offset,len);
}

void fb_write(const void *buf, off_t offset, size_t len)
{
  int row = (offset/4)/_screen.width;
	int col = (offset/4)%_screen.width;
	_draw_rect(buf,col,row,len/4,1);

}

void init_device()
{
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
}
