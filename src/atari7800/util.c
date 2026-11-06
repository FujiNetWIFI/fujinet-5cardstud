#ifdef BUILD_ATARI7800

/**
 * @brief   Utility Functions
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <stdbool.h>
#include <stdint.h>
#include <fujinet-fuji.h>
#include <fujinet-atari7800.h>
#include "maria.h"
#include "vars.h"
#include "../platform-specific/graphics.h"
#include "../platform-specific/util.h"

// READ HOST SLOTS returns all 8 slots.
#define FUJI_HOST_SLOT_COUNT 8
#define LOBBY_DEVICE_SLOT 0
#define LOBBY_MODE_READ   1

// MOUNT_IMAGE is answered only once the whole image is on the cart, which
// allows it 60 seconds; the extra 10 cover the round trip.
#define LOBBY_MOUNT_SECONDS 70

// The game timer counts in 60ths of a second: frames since the last
// resetTimer(), a PAL console's 50 a second scaled up.
static unsigned int timerBase;

void resetTimer(void)
{
  timerBase = frameCount;
}

int getTime(void)
{
  unsigned int t = frameCount - timerBase;

  if (mt_pal)
    t += t / 5;
  return (int) t;
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

/*
  SET_DEVICE_FULLPATH is a fixed 256-byte payload -- a short one is rejected on
  the ESP32 side -- so the path is padded here rather than at the call site.
*/
static const char lobbyPath[MAX_FILENAME_LEN] = "/atari7800/lobby.a78";
static const char lobbyHost[] = "ec.tnfs.io";

static HostSlot slots[FUJI_HOST_SLOT_COUNT];

static uint8_t findLobbyHost(void)
{
  uint8_t i;

  if (!fuji_get_host_slots(slots, FUJI_HOST_SLOT_COUNT))
    return FUJI_HOST_SLOT_COUNT;
  for (i = 0; i < FUJI_HOST_SLOT_COUNT; i++)
    if (sameHost(lobbyHost, (const char *) slots[i]))
      return i;
  return FUJI_HOST_SLOT_COUNT;
}

// The slot is used as found, never created: a player who reached this client
// through the FujiNet Lobby already has the host, and anyone else is told to
// add it in CONFIG.
static bool mountLobby(uint8_t slot)
{
  uint8_t timeout = fn_default_timeout;
  bool ok;

  if (!fuji_mount_host_slot(slot))
    return false;
  if (!fuji_set_device_filename(LOBBY_MODE_READ, slot, LOBBY_DEVICE_SLOT,
                                (char *) lobbyPath))
    return false;
  fn_default_timeout = LOBBY_MOUNT_SECONDS;
  ok = fuji_mount_disk_image(LOBBY_DEVICE_SLOT, LOBBY_MODE_READ);
  fn_default_timeout = timeout;
  return ok;
}

void quit(void)
{
  uint8_t slot;

  drawStatusText("LOADING LOBBY...");

  if (!fuji_a7800_present())
  {
    drawStatusText("NO FUJINET CARTRIDGE");
    return;
  }

  slot = findLobbyHost();
  if (slot == FUJI_HOST_SLOT_COUNT)
  {
    drawStatusText("ADD EC.TNFS.IO IN CONFIG");
    return;
  }

  if (!mountLobby(slot) || fuji_a7800_boot_state() != FUJI_A7800_BOOT_READY)
  {
    drawStatusText("LOBBY NOT AVAILABLE");
    return;
  }

  // Hand the console to the cartridge's loader. It does not return.
  fuji_a7800_boot();
}

#endif /* BUILD_ATARI7800 */
