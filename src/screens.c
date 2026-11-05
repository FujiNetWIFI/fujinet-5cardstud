#ifdef _CMOC_VERSION_
#include <fujinet-int.h>
#include <cmoc.h>
#include <coco.h>
unsigned char kbhit(void);
char cgetc(void);
#else
#include <stdlib.h>
#ifdef __WATCOMC__
//#define USE_PLATFORM_SPECIFIC_INPUT 1
#else
#include <conio.h>
#endif /* __WATCOMC__ */
#include <string.h>
#endif /* _CMOC_VERSION_ */
#include "stateclient.h"
#include "screens.h"
#include "misc.h"
#include "gamelogic.h"

#include "platform-specific/graphics.h"
#include "platform-specific/input.h"
#include "platform-specific/appkey.h"
#include "platform-specific/util.h"
#include "platform-specific/sound.h"

#define PLAYER_NAME_MAX 8

#ifdef HIRES_Y
unsigned char y_bump = 1;
#define BUMP_LINE y_bump+=4;
#define CLEAR_BUMP y_bump=1;
#else
#define BUMP_LINE y++;
#define CLEAR_BUMP
#endif

#define TABLE_LEFT (WIDTH/2-13)

// For limited height scenarios, set this (e.g. to 3) in platform specific vars.h
#ifndef TABLE_TOP
#define TABLE_TOP 5
#endif

// For limited height scenarios, set this to 1 to remove space between table names in platform specific vars.h
#ifndef TABLE_ROW_HEIGHT
#define TABLE_ROW_HEIGHT 2
#endif

// For limited height scenarios, set this to 0 or 1 in platform specific vars.h
// Also, "#define SKIP_FIRST_BUMP" in vars.h to avoid extra space at top
#ifndef HOW_TO_PLAY_ROW_START
#define HOW_TO_PLAY_ROW_START 3
#endif 

#ifndef drawTextCursor
#define drawTextCursor drawChip
#endif

unsigned char redrawGameScreen = 0;

/// @brief Convenience function to draw text centered at row Y
void centerText(unsigned char y, const char * text) {
#ifdef HIRES_Y
  drawTextAt((unsigned char)(WIDTH/2-strlen(text)/2), y*8+y_bump, text);
#else
  drawText((unsigned char)(WIDTH/2-strlen(text)/2), y, text);
#endif
}

/// @brief Convenience function to draw status text centered
void centerStatusText(const char * text) {
  clearStatusBar();
  drawStatusTextAt((unsigned char)((WIDTH-strlen(text))>>1),text);
}

/// @brief Convenience function to reset screen and draw border
void resetScreenWithBorder() {
  resetScreen();
  drawBorder();
}

/// @brief Shows information about the game
void showHelpScreen() {
  static unsigned char y;
  enableDoubleBuffer();
  centerStatusText(TXT("LOADING INSTRUCTIONS..", "Loading instructions.."));
  drawBuffer();

  // This COULD be retrieved from the server, especially if
  // this client were game agnostic.
  resetScreenWithBorder();
  
  centerText(HOW_TO_PLAY_ROW_START,TXT("HOW TO PLAY 5 CARD STUD", "How to play 5 Card Stud"));
  y=HOW_TO_PLAY_ROW_START+1;
  
  #ifndef SKIP_FIRST_BUMP
  BUMP_LINE
  #endif
                  //12345678901234567890123456789012
  y++;centerText(y, TXT("PLAYERS ARE DEALT 5 CARDS OVER", "Players are dealt 5 cards over"));
  y++;centerText(y, TXT("FOUR ROUNDS.  IN EVERY ROUND, ", "four rounds.  In every round, "));
  y++;centerText(y, TXT("PLAYERS BET, CALL, AND RAISE. ", "players bet, call, and raise. "));
  y+=2;
  
  centerText(y, TXT("MOVES", "Moves"));
  
  BUMP_LINE        //12345678901234567890123456789012
  y++;centerText(y, TXT("FOLD  - QUIT THE HAND        ", "Fold  - Quit the hand        "));BUMP_LINE
  y++;centerText(y, TXT("CHECK - FREE PASS            ", "Check - Free pass            "));BUMP_LINE
  y++;centerText(y, TXT("BET / - INCREASE BET. OTHERS ", "Bet / - Increase bet. Others "));
  y++;centerText(y, TXT("RAISE   MUST CALL TO STAY IN ", "Raise   must call to stay in "));BUMP_LINE
  y++;centerText(y, TXT("CALL  - MATCH THE CURRENT BET", "Call  - Match the current bet"));

  CLEAR_BUMP

#ifdef BUILD_COLECO
  centerStatusText("PRESS FIRE TO CONTINUE");
#elif defined(BUILD_NES)
  centerStatusText("PRESS A TO CONTINUE");
#elif defined(BUILD_SMS)
  centerStatusText("PRESS 1 TO CONTINUE");
#elif defined(BUILD_ATARI7800) || defined(BUILD_ATARI5200)
  centerStatusText("PRESS FIRE TO CONTINUE");
#else
  centerStatusText(TXT("PRESS A KEY TO CONTINUE", "Press a key to continue"));
#endif

  drawBuffer();

  clearCommonInput();
#ifdef USE_PLATFORM_SPECIFIC_INPUT
  // getPlatformKey() is non-blocking on these platforms
  while (!getPlatformKey());
#else
  cgetc();
#endif
  resetScreen();
  drawBuffer();
  redrawGameScreen=1;
}

/// @brief Action called in Welcome Screen to check if a server name is stored in an app key
void welcomeActionVerifyServerDetails() {
  // Read server endpoint into server and query
  read_appkey(AK_LOBBY_CREATOR_ID, AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER, (uint8_t *) tempBuffer);

  i=(unsigned char)strlen(tempBuffer);
  if (i) {

    // Split "server?query" into separate variables
    while(--i) {
      if (tempBuffer[i]=='?') {
        strcpy(query, tempBuffer+i);
        tempBuffer[i]=0;
        strcpy(serverEndpoint, tempBuffer);
        break;
      }
    }
  }
}




bool inputFieldCycle(uint8_t x, uint8_t y, uint8_t max, char* buffer) {
  static uint8_t done, curx, lastY;

  // Initialize first call to input box
  //i=strlen(buffer);
  if (done == 1 || lastY != y) {
    done=0;
    lastY=y;
    curx = (unsigned char)strlen((char *)buffer);
    drawText(x,y, (const char *)buffer);
    drawTextCursor(x+curx,y);
  }
   // curx=i;
   // done=0;
 // }


  // Process any waiting keystrokes
  #ifdef USE_PLATFORM_SPECIFIC_INPUT
  inputKey = getPlatformKey();
  if (inputKey) {
    done=0;
    disableDoubleBuffer();
  #else
  if (kbhit()) {
    done=0;
    disableDoubleBuffer();
    inputKey = cgetc();
  #endif

    if ((inputKey == KEY_RETURN ) && curx>1) {
      done=1;
      // remove cursor
      drawText(x+1+i,y," ");
    } else if (inputKey == KEY_BACKSPACE && curx>0) {
      buffer[--curx]=0;
      drawText(x+1+curx,y," ");
    } else if (
      curx < max && ((curx>0 && inputKey == KEY_SPACE) || (inputKey>= 97 && inputKey <=122) ||(inputKey>= 48 && inputKey <=57) || (inputKey>= 65 && inputKey <=90))    // 0-9 A-Z
    ) {
        buffer[curx]=(unsigned char)inputKey;
      buffer[++curx]=0;
    }

    drawText(x,y, (const char *)buffer);
    drawTextCursor(x+curx,y);

    return done==1;
  }

  return false;

}

void showPlayerNameScreen() {
  enableDoubleBuffer();
  resetScreenWithBorder();
  clearStatusBar();
  drawLogo();

  drawBuffer();
  disableDoubleBuffer();
  
  centerText(HEIGHT/2, TXT("ENTER YOUR NAME", "Enter your name"));
  drawBox(WIDTH/2-PLAYER_NAME_MAX/2-1,HEIGHT/2+2,PLAYER_NAME_MAX+1,1);
  drawText(WIDTH/2-PLAYER_NAME_MAX/2,HEIGHT/2+3, playerName);

  i=(unsigned char)strlen(playerName);

  clearCommonInput();
#ifdef USE_PLATFORM_NAME_ENTRY
  // No keyboard on this platform: the name is typed on screen instead.
  platformNameEntry(WIDTH/2-PLAYER_NAME_MAX/2, HEIGHT/2+3, PLAYER_NAME_MAX, playerName);
#else
  while (!inputFieldCycle(WIDTH/2-PLAYER_NAME_MAX/2, HEIGHT/2+3, PLAYER_NAME_MAX, playerName)) ;
#endif

  enableDoubleBuffer();
  for (y=HEIGHT/2;y<HEIGHT/2+5;++y)
    centerText(y, "                 ");

  drawBuffer();

  write_appkey(AK_LOBBY_CREATOR_ID, AK_LOBBY_APP_ID, AK_LOBBY_KEY_USERNAME, (uint8_t *) playerName);
}

/// @brief Action called in Welcome Screen to verify player has a name
void welcomeActionVerifyPlayerName() {
  // Read player's name from app key
  read_appkey(AK_LOBBY_CREATOR_ID, AK_LOBBY_APP_ID, AK_LOBBY_KEY_USERNAME, (uint8_t *) tempBuffer);

  tempBuffer[12]=0;
  if (tempBuffer[0] != 0)
    strcpy(playerName,tempBuffer);

  // Capture username if player didn't come in from the lobby
  if (strlen(playerName) == 0)
    showPlayerNameScreen();
}

/// @brief Shows the Welcome Screen with Logo. Asks player's name
void showWelcomeScreen() {
  resetScreenWithBorder();
  drawLogo();

  welcomeActionVerifyPlayerName();
  welcomeActionVerifyServerDetails();

  strcpy(tempBuffer, TXT("WELCOME ", "Welcome "));
  strcat(tempBuffer, playerName);
  centerText(HEIGHT/2,tempBuffer);
  drawBuffer();
  pause(45);


  // On first run, record the help as seen without showing it
  if (prefs[PREF_HELP]!=2) {
    prefs[PREF_HELP]=2;
    savePrefs();
    //showHelpScreen();

  }
  pause(30);
}

/// @brief Action in Table Selection Screen, joins the selected server
void tableActionJoinServer() {
  // Reset the game state
  clearGameState();

  strcat(query, "&player=");
  strcat(query, playerName);

  // Replace space with + for player name
  i=(unsigned char)strlen(query);
  while(--i)
    if (query[i]==' ')
      query[i]='+';

  redrawGameScreen=1;
}

/// @brief Shows a screen to select a table to join
void showTableSelectionScreen() {
  static uint8_t shownChip;
  static unsigned char tableIndex=0;
  static uint8_t skipApiCall;
  static Table* table;
  skipApiCall=0;
  // An empty query means a table needs to be selected
  while (strlen(query)==0) {

    // SkipApiCall will be true if toggling color, to redraw

    if (!skipApiCall) {
      // Show the status immediately before retrieval
      centerStatusText(TXT("REFRESHING TABLE LIST..", "Refreshing table list.."));
      drawBuffer();
    }

    resetScreenWithBorder();
    
    centerText(3, TXT("CHOOSE A TABLE TO JOIN", "Choose a table to join"));
    drawText(TABLE_LEFT,TABLE_TOP, TXT("TABLE", "Table"));
    drawText(TABLE_LEFT+19,TABLE_TOP, TXT("PLAYERS", "Players"));
    drawLine(TABLE_LEFT,TABLE_TOP+1,26);
    
    drawBuffer();
    waitvsync();

    if (!skipApiCall){
      apiCall("tables");
    }

    skipApiCall=0;

    if (clientState.tables.count>0) {
      for(i=0;i<clientState.tables.count;++i) {
        table = &clientState.tables.table[i];
#ifdef MIXED_CASE
        // The server sends table names in lower case
        for (j=0;table->name[j];++j)
          if (table->name[j]>='a' && table->name[j]<='z')
            table->name[j]-=32;
#endif
        drawText(TABLE_LEFT,TABLE_TOP+2+i*TABLE_ROW_HEIGHT, table->name);
        drawText((unsigned char)(TABLE_LEFT+26-strlen(table->players)), TABLE_TOP+2+i*TABLE_ROW_HEIGHT, table->players);
        
        if (table->players[0]>'0') {
          drawText((unsigned char)(TABLE_LEFT+24-strlen(table->players)), TABLE_TOP+2+i*TABLE_ROW_HEIGHT, "*");  
        }
      }
    } else {
      centerText(12, TXT("NO TABLES ARE AVAILABLE", "No tables are available"));
    }
    
    //drawStatusText(" R+EFRESH  H+ELP  C+OLOR  S+OUND  Q+UIT");
#if defined(TABLE_HINTS)
    drawStatusText(TABLE_HINTS);
#elif defined(BUILD_ATARI5200)
    // Keypad, not letters: src/atari5200/input.c maps 1-5 onto these shortcuts.
    //             1234567890123456789012345678901234567890
    drawStatusText("1-REFRESH 2-HELP 3-COLOR 4-NAME 5-QUIT");
#elif WIDTH>=40 
    drawStatusText("R-EFRESH   H-ELP  C-OLOR   N-AME   Q-UIT");
#elif defined(BUILD_COLECO)
    // Keypad, not letters: src/coleco/input.c maps 1-5 onto these shortcuts.
    //             12345678901234567890123456789012
    drawStatusText("1REFRSH 2HELP 3COLOR 4NAME 5QUIT");
#elif defined(BUILD_NES)
    // Buttons, not letters: src/nes/input.c maps them onto these shortcuts.
    //             12345678901234567890123456789012
    drawStatusText("A-JOIN  START-HELP  SELECT-NAME");
#elif defined(BUILD_SMS)
    // Buttons, not letters: src/sms/input.c maps them onto these shortcuts.
    //             12345678901234567890123456789012
    drawStatusText("1-JOIN <NAME >REFRESH PAUSE-HELP");
#elif defined(BUILD_ATARI7800)
    // Buttons and switches: src/atari7800/input.c maps them onto these shortcuts.
    //             12345678901234567890123456789012
    drawStatusText("1-JOIN  PAUSE-HELP  SELECT-NAME");
#else               //12345678901234567890123456789012
    drawStatusText("R-EFRESH   H-ELP   N-AME   Q-UIT");
#endif
    drawBuffer();
    disableDoubleBuffer();
    shownChip=0;

    clearCommonInput();
    while (!inputTrigger || !clientState.tables.count) {
      readCommonInput();
      //if (kbhit())
        //inputKey = cgetc();
      if (inputKey == 'h' || inputKey == 'H') {
        showHelpScreen();
        break;
      } else if (inputKey == 'r' || inputKey =='R') {
        break;
#ifndef NO_COLOR
      } else if (inputKey == 'c' || inputKey =='C') {
        prefs[PREF_COLOR] = cycleNextColor()+1;
        savePrefs();
        enableDoubleBuffer();
        skipApiCall=1;
        break;
#endif
        } else if (inputKey == 'n' || inputKey =='N') {
        showPlayerNameScreen();
        break;
      } else if (inputKey == 'q' || inputKey =='Q') {
        quit();
      } /*else if (inputKey != 0) {
        itoa(inputKey, tempBuffer, 10);
        drawStatusText(tempBuffer);
      } */

      if (clientState.tables.count>0 && (!shownChip || inputDirY)) {

        drawText(TABLE_LEFT-1,TABLE_TOP+2+tableIndex*TABLE_ROW_HEIGHT," ");
        tableIndex+=inputDirY;
        if (tableIndex==255)
          tableIndex=clientState.tables.count-1;
        else if (tableIndex>=clientState.tables.count)
          tableIndex=0;

        drawChip(TABLE_LEFT-1,TABLE_TOP+2+tableIndex*TABLE_ROW_HEIGHT);

        soundCursor();
        shownChip=1;
      }
    }

    enableDoubleBuffer();

    if (inputTrigger) {
      soundSelectMove();

      // Clear screen and write server name
      resetScreenWithBorder();
      clearStatusBar();
      centerText(HEIGHT/2, clientState.tables.table[tableIndex].name);

      strcpy(query, "?table=");
      strcat(query, clientState.tables.table[tableIndex].table);
      strcpy(tempBuffer, serverEndpoint);
      strcat(tempBuffer, query);

      //  Update server app key in case of reboot
      write_appkey(AK_LOBBY_CREATOR_ID, AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER, (uint8_t *) tempBuffer);

    }
  
  }

  centerText(HEIGHT-7, TXT("CONNECTING TO SERVER", "Connecting to server"));
  drawBuffer();

  progressAnim(HEIGHT-5);

  tableActionJoinServer();
}

/// @brief Shows main game play screen (table and cards)
void showGameScreen() {
  /*

typedef struct {
  char name   [9];
  uint8_t     status;
  uint16_t    bet;
  char move   [8];
  uint16_t    purse;
  char hand   [11];
} Player;

typedef struct {
  char lastResult[81];
  uint8_t round;
  uint16_t pot;
  int8_t activePlayer;
  uint8_t moveTime;
  uint8_t viewing;
  uint8_t validMoveCount;
  ValidMove validMoves[5];
  uint8_t playerCount;
  Player players[8];
} Game;
 */
  // printf("\r\nround=%u, pot=%u, ap=%s, m=%u,v=%u",state.round, state.pot, state.activePlayer, state.moveTime, state.viewing);
  // printf("\r\nvalidMoves=%u, playerCount=%u\r\n",state.validMoveCount, state.playerCount);

  // for(i=0;i<state.validMoveCount;++i) {
  //   printf(" %s=%s,",state.validMoves[i].move, state.validMoves[i].name);
  // }
  // printf("\r\n");
  // for(i=0;i<state.playerCount;++i) {
  //   printf("\r\n%s=%u, bet=%u |%s| p=%u, H=%s",state.players[i].name, state.players[i].status, state.players[i].bet, state.players[i].move, state.players[i].purse, state.players[i].hand);
  // }

  // printf("\r\n");
  // drawBuffer();
  // cgetc();

#ifndef SINGLE_BUFFER
  redrawGameScreen=1;
#endif

  checkIfSpectatorStatusChanged();
  checkIfPlayerCountChanged();

  // Animate chips to pot before drawing the new state
  animateChipsToPotOnRoundEnd();

  checkFinalFlip();

  if (redrawGameScreen) {
    resetScreen();
  }

  resetStateIfNewGame();

  drawPot();

  if (state.playerCount>1) {
    drawNamePurse();
    drawBets();
#ifdef SINGLE_BUFFER
    if (state.round < 5) {
      drawCards(false);
    }
#else
    drawCards(false);
#endif
  }

  drawGameStatus();
  drawBuffer();
  highlightActivePlayer();

#ifdef SINGLE_BUFFER
  redrawGameScreen=0;
#endif
  prevRound = state.round;
}

/// @brief shows in-game menu
void showInGameMenuScreen() {

  hideLine(0,0,0);
  i=1;
  while (i) {
    enableDoubleBuffer();
    clearStatusBar();
    resetScreenWithBorder();
    drawBuffer();

    x = WIDTH/2-9;
    y = HEIGHT/2-3;

#ifdef NO_COLOR
    drawBox(x-3,y-2,22,7);
#else
    drawBox(x-3,y-2,22,9);
#endif
#ifdef BUILD_COLECO
    drawText(x,y,    "  5: QUIT TABLE");
    drawText(x,y+=2, "  2: HOW TO PLAY");
    drawText(x,y+=2, "  3: COLOR TOGGLE");
    drawText(x,y+=2, "  *: KEEP PLAYING");
#elif defined(BUILD_NES)
    drawText(x,y,    "SEL+START: QUIT TABLE");
    drawText(x,y+=2, "START: HOW TO PLAY");
    drawText(x,y+=2, "B: KEEP PLAYING");
#elif defined(BUILD_SMS)
    drawText(x,y,    "  2+1: QUIT TABLE");
    drawText(x,y+=2, "PAUSE: HOW TO PLAY");
    drawText(x,y+=2, "    2: KEEP PLAYING");
#elif defined(BUILD_ATARI7800)
    drawText(x-2,y,    "SEL+PAUSE: QUIT TABLE");
    drawText(x-2,y+=2, "PAUSE: HOW TO PLAY");
    drawText(x-2,y+=2, "2: KEEP PLAYING");
#elif defined(BUILD_ATARI5200)
    // The font has no * or #: RESET is the labeled key that backs out.
    drawText(x,y,    "    5: QUIT TABLE");
    drawText(x,y+=2, "    2: HOW TO PLAY");
    drawText(x,y+=2, "    3: TABLE COLOR");
    drawText(x,y+=2, "RESET: KEEP PLAYING");
#else
    drawText(x,y,    TXT("  Q: QUIT TABLE", "  Q: Quit table"));
    drawText(x,y+=2, TXT("  H: HOW TO PLAY", "  H: How to play"));
#ifndef NO_COLOR
    drawText(x,y+=2, "  C: COLOR TOGGLE");
#endif
    //drawText(x,y+=2, "  S: SOUND TOGGLE");
    drawText(x,y+=2, TXT("ESC: KEEP PLAYING", "ESC: Keep playing"));
#endif
    drawBuffer();

    clearCommonInput();

    i=1;
    while (i==1) {
      readCommonInput();
      switch (inputKey) {
#ifndef NO_COLOR
        case 'c':
        case 'C':
            prefs[PREF_COLOR] = cycleNextColor()+1;
            savePrefs();
            i=2;
            break;
#endif
        case 'h':
        case 'H':
          showHelpScreen();
        case KEY_ESCAPE:
        case KEY_ESCAPE_ALT:
          i=0;
          break;
        case 'q':
        case 'Q':
          resetScreenWithBorder();
          centerText(10, TXT("PLEASE WAIT", "Please wait"));
          drawBuffer();

          // Inform server player is leaving
          apiCall("leave");
          progressAnim(12);
          
          //  Clear server app key in case of reboot
          write_appkey(AK_LOBBY_CREATOR_ID, AK_LOBBY_APP_ID, AK_LOBBY_KEY_SERVER, (uint8_t *) "");

          // Clear query so a new table will be selected
          clearCommonInput();
          strcpy(query,"");
          showTableSelectionScreen();
          return;
      }
    }
  }

  // Show game screen again before returning
  redrawGameScreen=1;
  resetScreen();
  drawBuffer();
  showGameScreen();
  clearCommonInput();
}
