#ifdef __TRS80M2__

// Five 4-wide cards take 20 columns. Seats sit in three tiers down each side,
// with the right side growing leftward from the right edge.
unsigned char playerXMaster[] = { 30, 2, 2, 2, 30, 74, 74, 74 };
unsigned char playerYMaster[] = { 17, 17, 10, 3, 3, 3, 10, 17 };

char playerDirMaster[] = { 1, 1, 1, 1, 1, -1, -1, -1 };
char playerBetXMaster[] = { 8, 10, 10, 10, 8, -6, -6, -6 };
char playerBetYMaster[] = { -2, -2, -2, -2, 4, -2, -2, -2 };

// Seats to use for each player count, indexes into the arrays above
//                               2                3                4
char playerCountIndex[] = {0,4,0,0,0,0,0,0, 0,2,6,0,0,0,0,0, 0,2,4,6,0,0,0,0,
// 5                6                 7                8
   0,2,3,5,6,0,0,0, 0,2,3,4,5,6,0,0,  0,2,3,4,5,6,7,0, 0,1,2,3,4,5,6,7};

#endif /* __TRS80M2__ */
