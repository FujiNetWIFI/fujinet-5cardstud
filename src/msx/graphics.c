/**
 * @brief   MSX Graphics Routines for 5cardstud
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#ifdef BUILD_MSX

#include <stdbool.h>
#include <video/tms99x8.h>
#include <conio.h>
#include <sys/ioctl.h>
#include "udg.h"
#include "vars.h"
#include "../platform-specific/graphics.h"

#define CORNER_TOP 0
#define CORNER_BOTTOM 18

bool always_render_full_cards = 0;

/**
 * @brief Initialize graphics mode; set palette.
 */
void initGraphics()
{
    void *param = &udg;
    vdp_set_mode(2);
    console_ioctl(IOCTL_GENCON_SET_UDGS,&param);
    vdp_color(VDP_INK_BLACK,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
    clrscr();
}

void drawChip(unsigned char x, unsigned char y)
{
    vdp_color(VDP_INK_DARK_RED,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
    gotoxy(x,y);
    cputc(0xBC);
}

void drawBuffer()
{
}

void drawBox(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned char i=0;
    // Correct coordinates;
    w++;
    h++;

    vdp_color(VDP_INK_DARK_RED,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);

    // Put box corners at coordinate extents
    gotoxy(x,y);
    cputc(0xa4);
    gotoxy(x+w,y);
    cputc(0xa5);
    gotoxy(x,y+h);
    cputc(0xa7);
    gotoxy(x+w,y+h);
    cputc(0xA8);

    x++;
    w--;

    // Put horizontal rules
    gotoxy(x,y);
    for (i=0;i<w;i++)
        cputc(0xA6);

    gotoxy(x,y+h);
    for (i=0;i<w;i++)
        cputc(0xA6);

    // Correct again
    x--;
    y++;
    h--;
    w++;

    // Put vertical rules
    for (i=0;i<h;i++)
    {
        gotoxy(x,y+i);
        cputc(0xA9);
    }

    for (i=0;i<h;i++)
    {
        gotoxy(x+w,y+i);
        cputc(0xA9);
    }
}

/**
 * @brief Draw text s at position x,y
 * @param x Horizontal position (0-31)
 * @param y Vertical Position (0-23)
 * @param s NULL terminated string to display.
 */
void drawText(unsigned char x, unsigned char y, const char* s)
{
    vdp_color(VDP_INK_BLACK,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
    gotoxy(x,y);
    cputs(strupr(s));
}

/**
 * @brief Draw the 5 Card Stud Logo
 */
void drawLogo()
{
    static unsigned char i;
    i=4;
    drawText(WIDTH/2-5,++i, "           ");
    drawText(WIDTH/2-5,++i, " FUJI  NET ");
    drawText(WIDTH/2-5,++i, "           ");
    drawText(WIDTH/2-5,++i, "5 CARD STUD");
    drawText(WIDTH/2-5,++i, "           ");
}

void clearStatusBar()
{
    vdp_vfill(0x1700,0x00,0x100);
    //vdp_vfill(MODE2_ATTR+0x1700,0xF0+VDP_INK_LIGHT_GREEN,0x100);
}

/**
 * @brief Clear the screen
 */
void resetScreen()
{
    vdp_color(VDP_INK_BLACK,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
    clrscr();
}

void disableDoubleBuffer()
{
}

void enableDoubleBuffer()
{
}

/**
 * @brief Draw card at position x,y
 * @param horizontal Card position (0-31)
 * @param vertical Card position (0-24)
 * @param partial enum (see ../platform-specific/graphics.h)
 * @param s String indicating number and suit. (e.g. "as" for ace of spades)
 * @param isHidden is card currently overturned?
 */
void drawCard(unsigned char x, unsigned char y, unsigned char partial, const char* s, unsigned char isHidden)
{
    static unsigned char val, suitColor, i, suit,rightDigit, peekChar, peekChar2;

    if (x==WIDTH-3 && s[0]!='?' && cvpeek(x,y+1)==0x9E) {
        drawCard(x+1,y,PARTIAL_RIGHT,"??",false);
    }

    if (partial == PARTIAL_LEFT)
    {
        gotoxy(x,y++);
        vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
        cputs("\x80");

        gotoxy(x,y++);
        vdp_color(VDP_INK_DARK_BLUE,VDP_INK_WHITE,VDP_INK_LIGHT_GREEN);
        cputs("\x9E");

        gotoxy(x,y++);
        vdp_color(VDP_INK_DARK_BLUE,VDP_INK_WHITE,VDP_INK_LIGHT_GREEN);
        cputs("\xA0");

        gotoxy(x,y++);
        vdp_color(VDP_INK_DARK_BLUE,VDP_INK_WHITE,VDP_INK_LIGHT_GREEN);
        cputs("\xA2");

        gotoxy(x,y++);
        vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
        cputs("\x81");
        
    }
    else if (partial == PARTIAL_RIGHT)
    {
        x++;
        gotoxy(x,y++);
        vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
        cputs("\x82");

        gotoxy(x,y++);
        vdp_color(VDP_INK_DARK_BLUE,VDP_INK_WHITE,VDP_INK_LIGHT_GREEN);
        cputs("\x9C");

        gotoxy(x,y++);
        vdp_color(VDP_INK_DARK_BLUE,VDP_INK_WHITE,VDP_INK_LIGHT_GREEN);
        cputs("\x9B");

        gotoxy(x,y++);
        vdp_color(VDP_INK_DARK_BLUE,VDP_INK_WHITE,VDP_INK_LIGHT_GREEN);
        cputs("\x9C");

        gotoxy(x,y++);
        vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
        cputs("\x85");
    }
    else // FULL CARD
    {
        switch (s[1])
        {
        case 'h' :
            suit=0xBA;
            suitColor=VDP_INK_DARK_RED;
            break;
        case 'd' :
            suit=0xB9;
            suitColor=VDP_INK_DARK_RED;
            break;
        case 'c' :
            suit=0xB8;
            suitColor=VDP_INK_BLACK;
            break;
        case 's' :
            suit=0xB7;
            suitColor=VDP_INK_BLACK;
            break;
        default:
            suit=0x86;
            suitColor=VDP_INK_DARK_YELLOW; // Something wrong.
            break;
        }

        // If card is overturned, draw the back
        if (s[0]=='?')
        {
            // Shift right card left one for easy drawing of border
            // As well as clear existing cards, assuming a fold
            if (x>WIDTH-3) {
                for (i=0;i<5;i++) {
                    gotoxy(x-7,y+i);
                    cputs("      ");
                }
                x--;
            } else {
                for (i=0;i<5;i++) {
                    gotoxy(x+3,y+i);
                    cputs("       ");
                }
            }
            gotoxy(x,y++); 
            vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN); 
            cputs("\x80\x82\x97"); 

            gotoxy(x,y++); 
            vdp_color(VDP_INK_DARK_BLUE,VDP_INK_WHITE,VDP_INK_LIGHT_GREEN); 
            cputs("\x9E\x9f"); 
            vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN); 
            cputc(0x86); 

            gotoxy(x,y++); 
            vdp_color(VDP_INK_DARK_BLUE,VDP_INK_WHITE,VDP_INK_LIGHT_GREEN); 
            cputs("\xA0\xA1"); 
            vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN); 
            cputc(0x86); 

            gotoxy(x,y++); 
            vdp_color(VDP_INK_DARK_BLUE,VDP_INK_WHITE,VDP_INK_LIGHT_GREEN); 
            cputs("\xa2\xa3"); 
            vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN); 
            cputc(0x86); 

            gotoxy(x,y++); 
            cputs("\x81\x83\x96");
            
        }
        else // Draw the full card.
        {
            unsigned char cardBkg=0;
            gotoxy(x,y);

            if (isHidden)
                cardBkg = VDP_INK_GRAY;
            else
                cardBkg = VDP_INK_WHITE;

            // Top card border
            if (cvpeek(x+1,y)!=0x82)
            {
                vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
                cputs("\x80\x82");
            }
            if (cvpeek(x+2,y)==' ')
            {
                vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
                cputc(0x97);
            }

            // left border and card value
            switch (s[0])
            {
            case 't':
                val=0xB2; // 10
                break;
            case 'j':
                val=0xB3;
                break;
            case 'q':
                val=0xB4;
                break;
            case 'k':
                val=0xB5;
                break;
            case 'a':
                val=0xB6;
                break;
            default:
                val=0xAA+(s[0]-'2');
                break;
            }

            // Left border/space
            gotoxy(x,++y);
            vdp_color(VDP_INK_DARK_BLUE,cardBkg,VDP_INK_LIGHT_GREEN);
            cputc(0x86);

            // Card value
            vdp_color(suitColor,cardBkg,VDP_INK_LIGHT_GREEN);
            cputc(val);
            
            // Right border (if no existing char)
            if (cvpeek(x+2,y)==' ')
            {
                vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
                cputc(0x86);
            }
            

            // left border and empty spaces
            gotoxy(x,++y);
            vdp_color(VDP_INK_DARK_BLUE,cardBkg,VDP_INK_LIGHT_GREEN);
            cputc(0x86);
            cputc(0x20);
            
            // Right border (if no existing char)
            if (cvpeek(x+2,y)==' ')
            {
                vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
                cputc(0x86);
            }
            

            // left border
            gotoxy(x,++y);
            vdp_color(VDP_INK_DARK_BLUE,cardBkg,VDP_INK_LIGHT_GREEN);
            cputc(0x86);

            // suit
            vdp_color(suitColor,cardBkg,VDP_INK_LIGHT_GREEN);
            cputc(suit);
            vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);

            // Right border (if no existing char)
            if (cvpeek(x+2,y)==' ')
            {
                vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
                cputc(0x86);
            }
            
            // bottom border
            gotoxy(x,++y);
            if (cvpeek(x+1,y)!=0x83)
            {
                vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
                cputs("\x81\x83");
            }
            if (cvpeek(x+2,y)==' ')
            {
                vdp_color(VDP_INK_DARK_BLUE,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
                cputc(0x96);
            }
        }
    }
}

void drawStatusText(const char* s)
{
    clearStatusBar();
    drawStatusTextAt(0, s);
    //vdp_color(VDP_INK_BLACK,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
    //gotoxy(0,22+(strlen(s)<=WIDTH?1:0));
    //cputs(strupr(s));
}

void drawStatusTextAt(unsigned char x, const char* s)
{
    vdp_color(VDP_INK_BLACK,VDP_INK_LIGHT_GREEN,VDP_INK_LIGHT_GREEN);
    gotoxy(x,22+(strlen(s)<=WIDTH?1:0));
    cputs(strupr(s));
}

unsigned char cycleNextColor()
{
}

void drawStatusTimer()
{
}

void hideLine(unsigned char x, unsigned char y, unsigned char w)
{
    uint8_t i;
    if (y==23)
    {
        vdp_vfill(MODE2_ATTR+0x1700+x*8,VDP_INK_BLACK*0x10+VDP_INK_LIGHT_GREEN,8*w);
    }
    else
    {
        for(i=0;i<w*8;i+=8)
        {
            vdp_vfill(MODE2_ATTR+0x100*y+x*8+i,0x10+VDP_INK_LIGHT_GREEN,2);
        }
    }

}

void drawLine(unsigned char x, unsigned char y, unsigned char w)
{
    uint8_t i;
    if (y==23) 
    {
        for(i=0;i<w*8;i+=8)
        {
            vdp_vpoke(MODE2_ATTR+0x1707+x*8+i,0x10+VDP_INK_MEDIUM_RED);
        }
    }
    else
    {
        for(i=0;i<w*8;i+=8) 
        {
            vdp_vfill(MODE2_ATTR+0x100*y+x*8+i,0x10+VDP_INK_MEDIUM_RED,2);
        }
    }
}

void setColorMode(unsigned char mode)
{
}

void drawBorder()
{
    drawCard(1,CORNER_TOP,FULL_CARD, "as", 0);
    drawCard(WIDTH-3,CORNER_TOP,FULL_CARD, "ah", 0);
    drawCard(1,CORNER_BOTTOM,FULL_CARD, "ad", 0);
    drawCard(WIDTH-3,CORNER_BOTTOM,FULL_CARD, "ac", 0);
}

#endif /* BUILD_MSX */
