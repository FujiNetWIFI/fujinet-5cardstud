#include <dos.h>
#include <i86.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <process.h>
#include <direct.h>
#include <stdio.h>
#include <fujinet-fuji.h>

extern unsigned char oldmode;

#define FUJI_SIGNATURE   "FUJI"
#define FUJI_HOST_SLOT_COUNT   8
#define FUJI_DEVICE_SLOT_COUNT 8

static HostSlot   host_slots[FUJI_HOST_SLOT_COUNT];
static DeviceSlot device_slots[FUJI_DEVICE_SLOT_COUNT];

typedef struct {
    uint8_t query;
    char    signature[4];
    uint8_t unit;
} fuji_ioctl_query;

static int find_slot_drive(int slot)
{
    int drive;
    union REGS regs;
    struct SREGS sregs;
    fuji_ioctl_query query;

    for (drive = 3; drive <= 26; drive++) {
        memset(&query, 0, sizeof(query));
        regs.h.ah = 0x44;
        regs.h.al = 0x04;
        regs.h.bl = (unsigned char)drive;
        regs.w.cx = sizeof(query);
        regs.x.dx = FP_OFF(&query);
        sregs.ds  = FP_SEG(&query);
        int86x(0x21, &regs, &regs, &sregs);
        if (!(regs.x.cflag & INTR_CF) &&
            memcmp(query.signature, FUJI_SIGNATURE, 4) == 0 &&
            query.unit == slot)
            return drive;
    }
    return -1;
}

static void mount(void)
{
    static uint8_t i, slot;
    unsigned int total;
    int drive;
    char host[]     = "ec.tnfs.io";
    char filename[] = "msdos/lobby.img";

    fuji_get_host_slots((unsigned char *)host_slots, FUJI_HOST_SLOT_COUNT);

    slot = FUJI_HOST_SLOT_COUNT;
    for (i = 0; i < FUJI_HOST_SLOT_COUNT; i++) {
        if (stricmp(host, (char *)host_slots[i]) == 0) {
            slot = i;
            break;
        }
    }

    if (slot == FUJI_HOST_SLOT_COUNT) {
        slot = FUJI_HOST_SLOT_COUNT - 1;
        strcpy((char *)host_slots[slot], host);
        fuji_put_host_slots((unsigned char *)host_slots, FUJI_HOST_SLOT_COUNT);
    }

    fuji_mount_host_slot(slot);

    device_slots[0].hostSlot = slot;
    device_slots[0].mode = 0;
    strcpy((char *)device_slots[0].file, filename);
    fuji_put_device_slots(device_slots, FUJI_DEVICE_SLOT_COUNT);
    fuji_set_device_filename(0, slot, 0, filename);
    fuji_mount_disk_image(0, 1);

    drive = find_slot_drive(0);
    if (drive < 0)
        return;

    _dos_setdrive(drive, &total);
    chdir("\\");
    spawnlp(P_OVERLAY, "COMMAND.COM", "COMMAND.COM", "/C", "AUTOEXEC.BAT", NULL);
}

// Screen specific player/bet coordinates
unsigned char playerXMaster[] = { 17,1, 1, 1, 15, 37,37, 37 };
unsigned char playerYMaster[] = { 18, 17, 10, 3, 2,3,10,17 };

char playerDirMaster[] = { 1,1,1,1,1,-1,-1,-1 };
char playerBetXMaster[] = { 1,10,10,10,3,-8,-8,-8 };
char playerBetYMaster[] = { -2, -2, 1,4,5,4,1,-2 };

// Simple hard coded arrangment of players around the table based on player count.
// These refer to index positions in the Master arrays above
// Downside is new players will cause existing player positions to move.

//                               2                3                4
char playerCountIndex[] = {0,4,0,0,0,0,0,0, 0,2,6,0,0,0,0,0, 0,2,4,6,0,0,0,0,
// 5                6                 7                8
    0,2,3,5,6,0,0,0, 0,2,3,4,5,6,0,0,  0,2,3,4,5,6,7,0, 0,1,2,3,4,5,6,7};

// /**
//  * @brief attempt to make a pause for given # of milliseconds,
//  * @param ms Milliseconds to wait, rounded to nearest 16.67ms frame.
//  */
// void pause(int ms)
// {
//     int frames = ms >> 4; // / 16, almost 16.67

//     while (frames--)
//     {
//         // Wait until vblank starts
//         while(!(inp(0x3DA) & 0x08));
//         // Wait until vblank stops
// 	    while(inp(0x3DA) & 0x08);
//     }
// }


static clock_t startClock;

void resetTimer() {
  startClock = clock();
}

int getTime() {
  return (int)((clock() - startClock) * 60 / CLOCKS_PER_SEC);
}

void quit()
{
	union REGS r;

	r.h.ah = 0x00;
	r.h.al = oldmode;
	int86(0x10,&r,&r);

	printf("Loading Lobby...\n");
	mount();
}
