#ifdef BUILD_MSX

/**
 * @brief   Utility Functions
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <fujinet-fuji.h>
#include "../platform-specific/graphics.h"

// The firmware's WRITE DEVICE SLOTS expects all 8 firmware slots even though
// the UI only surfaces 4, so keep full-width tables here.
#define FUJI_HOST_SLOT_COUNT 8
#define FUJI_DEVICE_SLOT_COUNT 8

// Where quit() hands the machine back to.
#define LOBBY_HOST "ec.tnfs.io"
#define LOBBY_FILE "msx/lobby.rom"

// FujiNet CONTROL register, top of the 0xBFFC IO window. 0x05 boots the user
// ROM, 0x04 boots BASIC.
#define FUJI_IO_CONTROL 0xBFFF
#define FUJI_BOOT_USER_ROM 0x05

// Scratch RAM for the bank-switch trampoline (see bootUserRom).
#define TRAMPOLINE 0xC000

static HostSlot host_slots[FUJI_HOST_SLOT_COUNT];
static DeviceSlot device_slots[FUJI_DEVICE_SLOT_COUNT];

// MSX tick counter (BIOS JIFFY).
__at (0xFC9E) unsigned int tikcnt;

void resetTimer(void)
{
    tikcnt=0;
}

void waitvsync(void)
{
    unsigned int jiffy = tikcnt;
    do {
        // wait.
    } while (tikcnt == jiffy);
}

int getTime(void)
{
    return tikcnt;
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
  static const char host[] = LOBBY_HOST;
  static const char filename[] = LOBBY_FILE;

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

/**
 * @brief Bank in the user ROM and cold start into it.
 *
 * The write to CONTROL pulls our own ROM out from under the program counter,
 * so the three instructions that do it have to be executing from RAM. Stage
 * them at 0xC000 and call through, exactly as fujinet-config's system_boot()
 * does (src/msx/system.c) - spelled as a function pointer here because sccz80
 * does not take SDCC's __asm/__endasm.
 *
 *   LD HL,$BFFF
 *   LD (HL),$05
 *   JP 0
 */
static void bootUserRom(void)
{
  unsigned char *code = (unsigned char *)TRAMPOLINE;

  code[0] = 0x21;
  code[1] = (unsigned char)(FUJI_IO_CONTROL & 0xFF);
  code[2] = (unsigned char)(FUJI_IO_CONTROL >> 8);
  code[3] = 0x36;
  code[4] = FUJI_BOOT_USER_ROM;
  code[5] = (unsigned char)0xC3;
  code[6] = 0x00;
  code[7] = 0x00;

  ((void (*)(void))TRAMPOLINE)();
}

void quit(void)
{
  drawStatusText("LOADING LOBBY...");
  mountLobby();

  // Same handoff fujinet-config uses: stop the firmware from serving CONFIG
  // at boot, then reboot so the machine comes up on the mounted image.
  fuji_set_boot_config(0);
  bootUserRom();
}

#endif /* BUILD_MSX */
