#include <fujinet-platform.h>

// c64 keeps its own, with the USE_EMULATOR path; ColecoVision its own reply-window read
#if !defined(FUJI_PLATFORM_C64) && !defined(FUJI_PLATFORM_COLECO)

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

#endif
