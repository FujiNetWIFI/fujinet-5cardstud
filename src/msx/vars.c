/**
 * @brief   Global variables for MSX
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details
 */

#ifdef BUILD_MSX

// 32x24 seat layout.
unsigned char playerXMaster[] = { 11, 0, 0, 0, 11, 30, 30, 30 };
unsigned char playerYMaster[] = { 18, 18, 10, 2, 2, 2, 10, 18 };

char playerDirMaster[] = { 1, 1, 1, 1, 1, -1, -1, -1 };
char playerBetXMaster[] = { 3, 10, 10, 10, 3, -8, -8, -8 };
char playerBetYMaster[] = { -4, -4, 0, 4, 4, 4, 0, -4 };

//                               2                3                4
char playerCountIndex[] = {0,4,0,0,0,0,0,0, 0,2,6,0,0,0,0,0, 0,2,4,6,0,0,0,0,
// 5                6                 7                8
    0,2,3,5,6,0,0,0, 0,2,3,4,5,6,0,0,  0,2,3,4,5,6,7,0, 0,1,2,3,4,5,6,7};

#endif /* BUILD_MSX */
