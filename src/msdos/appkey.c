/**
 * @brief MS-DOS AppKey funcs
 * @author Thom Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 * @license gpl, v. 3
 */

#include <stdint.h>
#include <string.h>
#include <fujinet-fuji.h>
#include "../platform-specific/appkey.h"

void read_appkey(unsigned int creator_id, unsigned char app_id, unsigned char key_id, char *data)
{
    uint16_t read = 0;

    fuji_set_appkey_details(creator_id, app_id, DEFAULT);
    if (!fuji_read_appkey(key_id, &read, (uint8_t *) data))
        read = 0;
    data[read] = 0;
}

void write_appkey(unsigned int creator_id, unsigned char app_id, unsigned char key_id, const char *data)
{
    fuji_set_appkey_details(creator_id, app_id, DEFAULT);
    fuji_write_appkey(key_id, strlen(data), (uint8_t *) data);
}
