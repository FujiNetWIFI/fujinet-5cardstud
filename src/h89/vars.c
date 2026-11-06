/**
 * @brief   Global variables for the Heathkit H89
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#ifdef __H89__

// 80x24 seat layout: the MS-DOS port's 40x25 one, twice as wide. Cards are
// 5x5 (src/h89/graphics.c). An opponent's hand is 19 columns, YOU's 21: the
// left seats' hands span columns 2-20, the right seats' 59-77 (a right-side
// seat's X is its first card, the rest fan out to the left), and the oval
// rail sits between them, rows 8-16, columns 22-57. Top seats (cards rows
// 2-7) and the bottom-centre seat (YOU, rows 18-22) are outside the rail
// above and below it. Each seat's move and bet lie on the felt just inside
// the rail nearest it; the pot is the box in the middle.
unsigned char playerXMaster[] = { 29, 2, 2, 2, 30, 73, 73, 73 };
unsigned char playerYMaster[] = { 18, 17, 10, 3, 2, 3, 10, 17 };

char playerDirMaster[] = { 1, 1, 1, 1, 1, -1, -1, -1 };
char playerBetXMaster[] = { 8, 22, 22, 22, 7, -16, -16, -16 };
char playerBetYMaster[] = { -4, -4, 1, 6, 7, 6, 1, -4 };

//                               2                3                4
char playerCountIndex[] = {0,4,0,0,0,0,0,0, 0,2,6,0,0,0,0,0, 0,2,4,6,0,0,0,0,
// 5                6                 7                8
    0,2,3,5,6,0,0,0, 0,2,3,4,5,6,0,0,  0,2,3,4,5,6,7,0, 0,1,2,3,4,5,6,7};

#endif /* __H89__ */
