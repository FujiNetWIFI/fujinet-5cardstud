#include <fujinet-platform.h>

// c64 keeps its own, with the USE_EMULATOR path
#ifndef FUJI_PLATFORM_C64

#include <fujinet-network.h>
#include "platform-specific/network.h"

uint8_t getResponse(const char *devicespec, void *buf, uint16_t len)
{
  int16_t count;

  if (network_open(devicespec, OPEN_MODE_HTTP_GET_H, OPEN_TRANS_NONE) != FN_ERR_OK)
    return 0;

  count = network_read(devicespec, buf, len);
  network_close(devicespec);

  return count > 0;
}

#endif /* FUJI_PLATFORM_C64 */
