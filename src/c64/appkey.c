#ifdef __C64__

/**
 * @brief   Cbm App Key Functions
 * @author  Eric Carr
 * @license gpl v. 3
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <cbm.h>
#include "../platform-specific/appkey.h"
#include "../misc.h"

#ifndef USE_EMULATOR

#include <fujinet-fuji.h>

void read_appkey(unsigned int creator_id, unsigned char app_id, unsigned char key_id, char* data)
{
  uint16_t read = 0;

  fuji_set_appkey_details(creator_id, app_id, DEFAULT);
  if (!fuji_read_appkey(key_id, &read, (uint8_t *)data))
    read = 0;

  // Add string terminator at end
  data[read] = 0;
}

void write_appkey(unsigned int creator_id, unsigned char app_id, unsigned char key_id, const char *data)
{
  fuji_set_appkey_details(creator_id, app_id, DEFAULT);
  fuji_write_appkey(key_id, strlen(data), (uint8_t *)data);
}

#else

#define LFN 15
#define DEV 11
#define OPEN_APPKEY_DELIM "-"

typedef struct {
  unsigned char command;
  appkeyblock payload; // The appkey (see appkey.h)
} appkeyC64;

static appkeyC64 appkey;

/**
 * @brief Open a key for reading or writing
 * @param open_mode 0 = read, 1 = write
 * @param creator_id Key creator ID: ($0000-$FFFF)
 * @param app_id The App ID ($00-$FF)
 * @param key_id The Key ID ($00-$FF)
 * @return error code
 */
unsigned char open_appkey(unsigned char open_mode, unsigned int creator_id, unsigned char app_id, char key_id)
{
  static unsigned char appkey_error;

  if (open_mode == APPKEY_WRITE)
    strcpy(appkey.payload.write.value, "s:");
  else
    strcpy(appkey.payload.write.value, "");

  itoa(creator_id, &appkey.payload.write.value[strlen(appkey.payload.write.value)], 16);

  strcat(appkey.payload.write.value, OPEN_APPKEY_DELIM);
  itoa(app_id, &appkey.payload.write.value[strlen(appkey.payload.write.value)], 16);

  strcat(appkey.payload.write.value, OPEN_APPKEY_DELIM);
  itoa(key_id, &appkey.payload.write.value[strlen(appkey.payload.write.value)], 16);

  strcat(appkey.payload.write.value, ".key");

  if (open_mode == APPKEY_WRITE) {
    appkey_error = cbm_open(LFN, DEV, 15, appkey.payload.write.value);
    cbm_close(LFN);

    appkey_error = cbm_open(LFN,DEV,open_mode,&appkey.payload.write.value[2]);
  } else {
    appkey_error = cbm_open(LFN,DEV,open_mode,&appkey.payload.write);
  }

  return appkey_error;
}

void read_appkey(unsigned int creator_id, unsigned char app_id, unsigned char key_id, char* data)
{
  static unsigned char appkey_error;
  static int read;

  // Set output as empty string
  data[0]=0;

  if (appkey_error = open_appkey(APPKEY_READ, creator_id, app_id, key_id))
    return;

  if (!appkey_error) {
    read = cbm_read(LFN, &appkey.payload, sizeof(appkey.payload.read));

    if (read>1 && appkey.payload.read.length <= MAX_APPKEY_LEN) {
        memcpy(data, appkey.payload.read.value, appkey.payload.read.length);
        data[appkey.payload.read.length] = 0;
    } else {
        appkey_error = 64;
    }

    // Future: may need to call close_appkey
  }

  cbm_close(LFN);
  return;
}

void write_appkey(unsigned int creator_id, unsigned char app_id, unsigned char key_id, const char *data)
{
  static unsigned char appkey_error;
  static unsigned char len;

  if (appkey_error = open_appkey(APPKEY_WRITE, creator_id, app_id, key_id))
    return;

  len = strlen(data);
  if (len > MAX_APPKEY_LEN)
    len = MAX_APPKEY_LEN;

  memset(&appkey.payload, 0, sizeof(appkey.payload.read));
  appkey.payload.read.length = len;
  memcpy(&appkey.payload.read.value, data, appkey.payload.read.length);

  cbm_write(LFN, &appkey.payload.read, sizeof(appkey.payload.read));

  cbm_close(LFN);

  // Future: may need to call close_appkey

  return;
}

#endif /* USE_EMULATOR */

#endif /* __C64__ */
