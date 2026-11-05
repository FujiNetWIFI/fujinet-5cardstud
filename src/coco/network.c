#ifdef _CMOC_VERSION_

#include <cmoc.h>
#include <coco.h>
#include <fujinet-network.h>
#include "../misc.h"

uint8_t getResponse(char *url, unsigned char *buffer, uint16_t max_len) {
    static int16_t count;

    if (network_open(url, OPEN_MODE_HTTP_GET_H, OPEN_TRANS_NONE) != FN_ERR_OK)
        return 0;
    count = network_read(url, buffer, max_len);
    network_close(url);
    return count > 0;
}

#endif /* _CMOC_VERSION_ */
