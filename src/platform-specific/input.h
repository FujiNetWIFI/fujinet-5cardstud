/*
  Platform specific input.
*/
#ifndef INPUT_H
#define INPUT_H

// Include platform specific defines before the input include
#include "../c64/vars.h"
#include "../apple2/vars.h"
#include "../adam/vars.h"
#include "../coco/vars.h"
#include "../msdos/vars.h"
#include "../plus4/vars.h"
#include "../msx/vars.h"
#include "../coleco/vars.h"

// Platform specific implementations
unsigned char readJoystick();

#ifdef USE_PLATFORM_SPECIFIC_INPUT
void initPlatformKeyboardInput(void);
int getPlatformKey(void);
#endif

// Platforms with no keyboard at all type on screen instead of through
// inputFieldCycle(); see src/coleco/osk.c.
#ifdef USE_PLATFORM_NAME_ENTRY
void platformNameEntry(unsigned char x, unsigned char y, unsigned char max,
                       char *buffer);
#endif

#endif /* INPUT_H */
