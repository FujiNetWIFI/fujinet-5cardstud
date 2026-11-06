#ifdef _CMOC_VERSION_
#include <cmoc.h>
#define true 1
#define false 0
#else
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#endif /* CMOC_VERSION */
#define POKE(addr,val)     (*(unsigned char*) (addr) = ((unsigned char)(val)))
#define POKEW(addr,val)    (*(unsigned*) (addr) = ((unsigned)(val)))
#define PEEK(addr)         (*(unsigned char*) (addr))
#define PEEKW(addr)        (*(unsigned*) (addr))
#include "stateclient.h"
#include "misc.h"
#include "platform-specific/network.h"
#ifndef _CMOC_VERSION_
#include <conio.h>
#endif

char urlBuffer[URL_BUFFER_LEN];

unsigned char apiCall(const char *path) {
#ifndef __CBM__
  if (serverEndpoint[0] != 'N') {
    strcpy(urlBuffer,"N:");
  } else {
    strcpy(urlBuffer,"");
  }
#else
  memset(urlBuffer,0,sizeof(urlBuffer));
#endif /* ! __CBM__ */

  strcat(urlBuffer, serverEndpoint);
  strcat(urlBuffer, path);
  strcat(urlBuffer, query);

  // Set Binary mode
  strcat(urlBuffer, query[0] ? "&bin=1" QUERY_SUFFIX : "?bin=1" QUERY_SUFFIX);
  //printf(urlBuffer);
  //printf("\r\n");
  // sizeof(Game), not sizeof(clientState.game): on the ColecoVision clientState
  // is a macro for a cast-and-dereference of the cartridge reply window, and
  // sccz80 will not take sizeof() of that expression.
  return getResponse(urlBuffer, &clientState.firstByte, sizeof(Game));

}

#ifdef MIXED_CASE
static void capitalize(char *s) {
  if (s[0]>='a' && s[0]<='z')
    s[0]-=32;
}

/* The server sends all text in lower case: the player's own name goes back to
   how it was typed, bots ("<name> bot") get capitals, and the result line
   follows suit. */
static void fixText() {
  static unsigned char p, n;
  static char *s, *r;

  for (p=0;p<state.validMoveCount && p<5;p++)
    capitalize(state.validMoves[p].name);

  for (p=0;p<state.playerCount && p<8;p++) {
    capitalize(state.players[p].move);

    s=state.players[p].name;
    if (!s[0])
      continue;

    for (n=0;s[n] && (s[n]|0x20)==(playerName[n]|0x20);n++)
      ;
    if (!s[n] && !playerName[n]) {
      strcpy(s,playerName);
    } else {
      n=(unsigned char)strlen(s);
      if (n>4 && !strcmp(s+n-4," bot")) {
        capitalize(s);
        s[n-3]='B';
      }
    }

    for (r=state.lastResult;*r;r++) {
      if (r!=state.lastResult && r[-1]!=' ')
        continue;
      for (n=0;s[n] && (r[n]|0x20)==(s[n]|0x20);n++)
        ;
      if (!s[n] && (r[n]==' ' || !r[n]))
        memcpy(r,s,n);
    }
  }
}
#endif

unsigned char getStateFromServer()
{
  if (requestedMove) {
    strcpy(tempBuffer, "move/");
    strcat(tempBuffer, requestedMove);
    requestedMove=NULL;
  } else {
    strcpy(tempBuffer, "state");
  }

#ifdef MIXED_CASE
  if (!apiCall(tempBuffer))
    return 0;
  fixText();
  return 1;
#else
  return apiCall(tempBuffer);
#endif
}
