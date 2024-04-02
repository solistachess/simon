//attack.cpp by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.
#include "chess.h"

/*********************************************************************
File includes initialization of the rotated bitboards (or attack
boards) plus functions to compute attacks using these boards.

The basic advantage of rotated attack boards is that they permit 
sliding piece attack calculations using bit operations (and, or, xor 
shift)  without the need to traverse the board.  Bit operations are 
fast whereas traversing the board (looping operations) are 
considerably slower.  The downside is that we have to maintain, in 
effect, 4 chess boards instead of just 1.  Every time we make or 
unmake a move all 4 boards have to be updated.

On first encounter the rotated boards seem rather like witchcraft but 
are really not that complicated once you understand them.  The 4 
boards are:

(1) ranks = ranks.
(2) ranks = files.
(3) ranks = diagonals along a8-h1
(4) ranks = diagonals along a1-h8

Board (1) is the normal chessboard.  Board (2) is a chessboard turned
sideways or rotated 90 degrees.  Boards (3) and (4) are less intuitive
since diagonal lengths vary from 1 to 8 whereas ranks and files are 
always length 8.  For board (3) we rotate the board 45 degrees to the
left so the a8-h1 diagonals form "ranks".  There are 15 ranks which 
vary in length from 1 to 8.  To build the rotated board we take the 
first rank (square a1) and place it at the start (square a1) of the 
rotated board.  The second rank (squares a2 and b1) follows in 
squares b1 and c1.  etc.  When done we have:

normal board rotated                      rotated left 45 degree
left 45 degrees:      h8                  board:
                    g8  h7
                  f8  g7  h6
                e8  f7  g6  h5            g6 h5 f8 g7 h6 g8 h7 h8
              d8  e7  f6  g5  h4          h3 d8 e7 f6 g5 h4 e8 f7
            c8  d7  e6  f5  g4  h3        f4 g3 h2 c8 d7 e6 f5 g4 
          b8  c7  d6  e5  f4  g3  h2      e4 f3 g2 h1 b8 c7 d6 e5
        a8  b7  c6  d5  e4  f3  g2  h1    d4 e3 f2 g1 a8 b7 c6 d5
          a7  b6  c5  d4  e3  f2  g1      b5 c4 d3 e2 f1 a7 b6 c5
            a6  b5  c4  d3  e2  f1        c2 d1 a5 b4 c3 d2 e1 a6
              a5  b4  c3  d2  e1          a1 a2 b1 a3 b2 c1 a4 b3
                a4  b3  c2  d1
                  a3  b2  c1
                    a2  b1
                      a1


Now, how they work.  An rrank (Tony the tiger would like that term) is
any rank on the rotated boards - thus an rrank is a rank, file or
diagonal on the actual board.  We can put any rrank in the first rank
position (ie make it's starting square coincide with square a1) by
a bit shift.  For example, to move actual rank 2 to rank 1 we do an
8 bit shift.  To move the a3-b2-c1 diagonal in the rl45 board to rank
1 takes a 3 bit shift.  Once the rrank is in a known position we can
examine it's contents - or compare them with what we already know.
  
The rotated attack boards are arrays of [square][rrank] which are
filled by the function InitAttack().  For each [square] it looks at 
all possible states of occupation of rrank.  For each occupation state 
it starts at [square] and goes left and right (this done by the helper
function GetAtk()) until bumps into something - either an occupied 
square or the end of the rrank.

Once we have all combinations, attacks are computed as follows.  The 
actual board position is bit shifted so the square of interest is on
the first rank.  We "and" out all the higher ranks leaving only the
state of occupation of our rrank.  The state of occupation of our 
rrank serves as an index into our pre-computed bitmaps of where we
can go and who we attack or defend.

An extremely important point to note here:  the pre-computed bitmap 
will include empty squares plus the first piece we bumped into NO
MATTER WHAT COLOR THAT PIECE MAY BE since the computation is based 
solely on occupation state.  We must use our bitboards to determine if
that piece is friend or foe, whether we attack or defend and whether 
we can move there.

A side note:  an rrank can have up to 8 squares which means 256 
possible states of occupation, yet the rrank array dimension is only
128.  That's because we simply throw away the (equivalent of) the h 
file.  We can do this because if a piece can move without obstruction
to the g file the h file is included in the bitmap - it doesn't 
matter if it is empty or occupied.  We could, in like manner, throw 
away the a file and reduce the dimension to 64 but that introduces 
some extra math (which the cache savings may or may not pay for) into 
the bit shift calculation.
*********************************************************************/

//====================================================================
//Attacks() produces a bitboard of all squares that attack b2.
//====================================================================
U64 Attacks(int b2) {
  return ((atk_wpawn(b2) & b_pawn) | (atk_bpawn(b2) & w_pawn) |
    (atk_knight(b2) & (w_knight | b_knight)) | 
    (atk_king(b2) & (w_king | b_king)) |
    (atk_bish(b2) & bish_queen) | (atk_rook(b2) & rook_queen));
}

//====================================================================
//Attacked() is used to determine if square b1 is attacked by color ka.
//Returns TRUE as soon as it finds any attack
//====================================================================
int Attacked(int b1, int ka) {
  if (ka) {
    if (atk_wpawn(b1) & b_pawn) return TRUE;
    if (atk_knight(b1) & b_knight) return TRUE;
    if (atk_bish(b1) & (b_bish | b_queen)) return TRUE;
    if (atk_rook(b1) & (b_rook | b_queen)) return TRUE;
    if (atk_king(b1) & b_king) return TRUE;
  } else {
    if (atk_bpawn(b1) & w_pawn) return TRUE;
    if (atk_knight(b1) & w_knight) return TRUE;
    if (atk_bish(b1) & (w_bish | w_queen)) return TRUE;
    if (atk_rook(b1) & (w_rook | w_queen)) return TRUE;
    if (atk_king(b1) & w_king) return TRUE;
  }
  return FALSE;
}

//====================================================================
//Sex() Static exchange.  a true static exchange function would look
//at all attackors and defenders, this just looks to see if the
//prospective capture is defended
//====================================================================
int Sex(int move) {
  int win = piece_value[mv_cap(move)];    //prospective capture winnings
  int lose = piece_value[mv_man(move)];   //what we stand to lose
  if (mv_pro(move)) {                     //steaks go up if promotion
    win += q_val - p_val;
    lose = q_val;
  }
  if (Attacked(mv_b2(move), color ^ KTC))
    return win - lose;                    //capture is defended
  else
    return win;                           //capture not defended
}

//=====================================================================
//GetAtk() gets attacks from file c1 on an rrank of length len with 
//occupation state j1.  It simply goes left and right setting bits 
//until it bumps into something.
//=====================================================================
static int GetAtk(int c1, int j1, int len) {
  int i1, a1=0;
  //find attacks to left of c1
  if (c1 > 0) {
    i1 = 1 << (c1 - 1);
    while (i1 > 0) {
      a1 = a1 | i1;
      if (j1 & i1) break;
      i1 = i1 >> 1;
    }
  }
  //find attacks to right of c1
  if (c1 < 7) {
    i1 = 1 << (c1 + 1);
    while (i1 < 256) {
      a1 = a1 | i1;
      if (j1 & i1) break;
      i1 = i1 << 1;
    }
  }
  //since we fill all combinations it doesn't matter what we drag 
  //along when the rrank is shifted to the first rank position, but
  //for the diagonals (most of which are less than 8 squares)
  //we can't have them showing up in the attack pattern so we mask
  //off anything beyond the length of the rrank.
  i1 = (1<<len) - 1;
  return (a1 & i1);
}

//====================================================================
//InitAttack() initialize directions[] and obstructed[] arrays and 
//the rotated attack boards.
//====================================================================
void InitAttack(void) {
  int i, j, b1, r1, c1, d1, len;
  U64 a1, mask;
  //rotated board geometry.
  char rl90[64] = {
    56, 48, 40, 32, 24, 16,  8, 0,
    57, 49, 41, 33, 25, 17,  9, 1,
    58, 50, 42, 34, 26, 18, 10, 2,
    59, 51, 43, 35, 27, 19, 11, 3,
    60, 52, 44, 36, 28, 20, 12, 4,
    61, 53, 45, 37, 29, 21, 13, 5,
    62, 54, 46, 38, 30, 22, 14, 6,
    63, 55, 47, 39, 31, 23, 15, 7};
  char rl45[64] = {                   //compare with comments above
     0,  8,  1, 16,  9,  2, 24, 17,   //a1 a2 b1 a3 b2 c1 a4 b3
    10,  3, 32, 25, 18, 11,  4, 40,   //c2 d1 a5 b4 c3 d2 e1 a6
    33, 26, 19, 12,  5, 48, 41, 34,   //etc.
    27, 20, 13,  6, 56, 49, 42, 35,
    28, 21, 14,  7, 57, 50, 43, 36,
    29, 22, 15, 58, 51, 44, 37, 30,
    23, 59, 52, 45, 38, 31, 60, 53,
    46, 39, 61, 54, 47, 62, 55, 63};
  char rr45[64] = {
     7,  6, 15,  5, 14, 23,  4, 13,
    22, 31,  3, 12, 21, 30, 39,  2,
    11, 20, 29, 38, 47,  1, 10, 19,
    28, 37, 46, 55,  0,  9, 18, 27,
    36, 45, 54, 63,  8, 17, 26, 35,
    44, 53, 62, 16, 25, 34, 43, 52,
    61, 24, 33, 42, 51, 60, 32, 41,
    50, 59, 40, 49, 58, 48, 57, 56};
  //amount squares are shifted to get rrank to rank 1
  char delta[64] = {
     0,  1,  1,  3,  3,  3,  6,  6,
     6,  6, 10, 10, 10, 10, 10, 15,
    15, 15, 15, 15, 15, 21, 21, 21,
    21, 21, 21, 21, 28, 28, 28, 28,
    28, 28, 28, 28, 36, 36, 36, 36,
    36, 36, 36, 43, 43, 43, 43, 43,
    43, 49, 49, 49, 49, 49, 54, 54,
    54, 54, 58, 58, 58, 61, 61, 63};
  //initialize directions[] and obstructed[]
  for (i=0; i<64; i++) {
    a1 = ap_queen[i];
    for (j=0; j<64; j++) {
      directions[i][j] = 0;
      obstructed[i][j] = all_64;
      if (a1 & sq_set[j]) {
        r1 = row(j) - row(i);
        if (r1) r1 = r1/abs_val(r1);
        c1 = col(j) - col(i);
        if (c1) c1 = c1/abs_val(c1);
        d1 = 8*r1 + c1;
        directions[i][j] = d1;
        mask = 0;
        b1 = i;
		// Loop until b1 == j
        for ( ; ; ) {
          b1 += d1;
          if (b1==j) break;
          mask |= sq_set[b1];
        }
        obstructed[i][j] = mask;
      }
    }
  }
  //initialize the rr00 (non rotated) attack board
  for (i = 0; i < 64; i++) {      //i = square on normal board
    for (j = 0; j < 128; j++) {   //j = state of occupation
      ap_rook_rr00[i][j] = 0;
      d1 = GetAtk(col(i), j, 8);  //attacks for occupation state
      while (d1) {                //extract bits from d1
        b1 = FirstBit(d1);        //and set on attack board
        ap_rook_rr00[i][j] |= sq_set[(i & 56)+b1];
        d1 ^= 1 << b1;
      }
    }
  }
  //initialize rl90 attack board
  for (i = 0; i < 64; i++) {
    for (j = 0; j < 128; j++) {
      ap_rook_rl90[i][j] = 0;
      d1 = GetAtk(7 - row(i), j, 8);
      while (d1) {
        b1 = FirstBit(d1);
        ap_rook_rl90[i][j] |= sq_set[rl90[(col(i)<<3)+b1]];
        d1 ^= 1 << b1;
      }
    }
  }
  //initialize the rl45 (a8-h1) attack board
  for (i = 0; i < 64; i++) {
    r1 = row(i);    //normal rank
    c1 = col(i);    //normal file
    len = (c1 < 8 - r1) ? r1 + c1 + 1 : 15 - r1 - c1;  //length
    c1 = c1 > 7 - r1 ? 7 - r1 : c1;     //rotated file
    r1 = FirstBit(sq_set_rl45[i]);      //rotated board square
    for (j = 0; j < 128; j++) {
      ap_bish_rl45[i][j] = 0;
      d1 = GetAtk(c1, j, len);
      while (d1) {
        b1 = FirstBit(d1);
        ap_bish_rl45[i][j] |= sq_set[rl45[b1 + delta[r1]]];
        d1 ^= 1 << b1;
      }
    }
  }
  //initialize the rr45 (a1-h8) attack board
  for (i = 0; i < 64; i++) {
    r1 = row(i);
    c1 = col(i);
    len = c1 < r1 ? 8 - r1 + c1 : 8 + r1 - c1;
    c1 = c1 > r1 ? r1 : c1;
    r1 = FirstBit(sq_set_rr45[i]);
    for (j = 0; j < 128; j++) {
      ap_bish_rr45[i][j] = 0;
      d1 = GetAtk(c1, j, len);
      while (d1) {
        b1 = FirstBit(d1);
        ap_bish_rr45[i][j] |= sq_set[rr45[b1 + delta[r1]]];
        d1 ^= 1 << b1;
      }
    }
  }
} //InitAttack()

