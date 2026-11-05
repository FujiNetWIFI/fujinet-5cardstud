/**
 * @brief   App key functions
 * @author  Oscar Fowler, Thomas Cherryhomes, Eric Carr
 * @license gpl v. 3
 */

#ifndef APPKEY_H
#define APPKEY_H

#include <fujinet-int.h>

#define MAX_APPKEY_LEN 64

#define APPKEY_READ  0
#define APPKEY_WRITE 1

/**
 * @brief Convenience type for handling an appkey
 */
typedef union {
    struct
    {
        unsigned int creator;
        unsigned char app;
        unsigned char key;
        unsigned char mode;
        unsigned char reserved;
    } open;
    struct
    {
        unsigned char length;
        unsigned char reserved;
        unsigned char value[MAX_APPKEY_LEN];

    } read;
    struct
    {
       char value[MAX_APPKEY_LEN+32];
    } write;
} appkeyblock;

// Reads the key into data as a C string, empty if it can't be read
void read_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, uint8_t *data);

// Writes data, a C string of at most MAX_APPKEY_LEN characters
void write_appkey(uint16_t creator_id, uint8_t app_id, uint8_t key_id, uint8_t *data);

#endif /* APPKEY_H */
