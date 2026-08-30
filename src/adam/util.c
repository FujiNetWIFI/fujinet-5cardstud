#ifdef __ADAM__

/**
 * @brief   Utility Functions
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <eos.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <fujinet-fuji.h>
#include "../platform-specific/graphics.h"

// The firmware's WRITE DEVICE SLOTS expects all 8 firmware slots even though
// the Adam UI only surfaces 4, so keep full-width tables here.
#define FUJI_HOST_SLOT_COUNT 8
#define FUJI_DEVICE_SLOT_COUNT 8

static HostSlot host_slots[FUJI_HOST_SLOT_COUNT];
static DeviceSlot device_slots[FUJI_DEVICE_SLOT_COUNT];

volatile unsigned long currentTime;
static volatile bool vsync;

// Installed as the VDP interrupt hook by initGraphics() via add_raster_int().
// Touches no VDP registers, so it cannot race the console's VRAM writes.
void myInt(void)
{
  currentTime++;
  vsync=true;
}

void waitvsync(void)
{
  while(!vsync);
  vsync=false;
}

void resetTimer(void)
{
  currentTime = 0;
}

int getTime(void)
{
  return currentTime;
}

static bool sameHost(const char *a, const char *b)
{
  while (*a && *b)
  {
    if ((*a | 0x20) != (*b | 0x20))
      return false;
    a++;
    b++;
  }
  return *a == *b;
}

static void mountLobby(void)
{
  static uint8_t i, slot;
  static const char host[] = "ec.tnfs.io";
  static const char filename[] = "adam/lobby.ddp";

  // Read current list of hosts from FujiNet
  fuji_get_host_slots(host_slots, FUJI_HOST_SLOT_COUNT);

  // Pick the host slot to use. Default to the last, but choose an existing
  // slot if it already has the same host
  slot = FUJI_HOST_SLOT_COUNT;
  for (i=0; i<FUJI_HOST_SLOT_COUNT; i++)
  {
    if (sameHost(host, (char *)host_slots[i]))
    {
      slot = i;
      break;
    }
  }

  // Update the bottom host slot if needed
  if (slot == FUJI_HOST_SLOT_COUNT)
  {
    slot = FUJI_HOST_SLOT_COUNT-1;
    strcpy((char *)host_slots[slot], host);
    fuji_put_host_slots(host_slots, FUJI_HOST_SLOT_COUNT);
  }

  fuji_mount_host_slot(slot);

  // Mount the lobby image to device slot 0
  device_slots[0].hostSlot = slot;
  device_slots[0].mode = 0;
  strcpy((char *)device_slots[0].file, filename);
  fuji_put_device_slots(device_slots, FUJI_DEVICE_SLOT_COUNT);
  fuji_set_device_filename(0, slot, 0, (char *)filename);
  fuji_mount_disk_image(0, 1);
}

void quit(void)
{
  drawStatusText("LOADING LOBBY...");
  mountLobby();

  // Same handoff fujinet-config uses: stop the firmware from serving CONFIG
  // at boot, then reinitialize EOS so the Adam boots the mounted image.
  fuji_set_boot_config(0);
  eos_init();
}

#endif /* __ADAM__ */
