#include <fujinet-platform.h>

// c64 keeps its own, with the USE_EMULATOR path
#ifndef FUJI_PLATFORM_C64

#ifdef _CMOC_VERSION_
#include <cmoc.h>
#else
#include <string.h>
#endif
#include <fujinet-fuji.h>
#include "platform-specific/appkey.h"

void read_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, uint8_t *data)
{
  uint16_t count = 0;

  fuji_set_appkey_details(creator_id, app_id, DEFAULT);
  if (!fuji_read_appkey(key_id, &count, data))
    count = 0;

  // Callers use the key as a C string
  data[count] = 0;
}

void write_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, uint8_t *data)
{
  fuji_set_appkey_details(creator_id, app_id, DEFAULT);
  fuji_write_appkey(key_id, strlen((char *) data), data);
}

#endif /* FUJI_PLATFORM_C64 */
