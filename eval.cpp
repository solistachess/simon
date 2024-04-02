//eval.cpp by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.
#include "chess.h"

/*********************************************************************
This file includes the static position evaluation function, Eval().
Evaluation begins with the basic material balance which we already
know since this is updated by Move(). To that value we add a couple of
bonuses and penaltys to show the mechanics.

Note that the evaluation proceeds from white's point of view - things
good for white get a plus and things good for black get a minus.  When
all is said and done we check the turn.  If it is black's turn the 
score is negated.
*********************************************************************/

static const U64 sweet_ctr = sq_set[E4] | sq_set[D4] | 
                             sq_set[E5] | sq_set[D5];

//====================================================================
//Center() returns proximity to the center
//====================================================================
int Center(int b1) {
  U64 sq = sq_set[b1];
  if (sq & sweet_ctr) return 8;
  if (sq & 0x00003c24243c0000) return 4;  //near center
  if (sq & 0x007e424242427e00) return 2;  //non center
  return 0;                               //rim
}
  
//====================================================================
//Eval() evaluates the position.  returns score positive if side
//moving stands better
//====================================================================
int Eval() {
  int b1, val;
  U64 men;
  //material
  val = white_mtl - black_mtl;
  //pawns
  men = w_pawn;
  val += 5 * BitCount(men & sweet_ctr);       //credit center pawns
  while (men) {
    b1 = FirstBit(men);
    men &= sq_clr[b1];    
    val += row(b1);                           //credit advancement
    if (mask_col[col(b1)] & men) val -= 10;   //penalty if doubled
  }
  men = b_pawn;
  val -= 5 * BitCount(men & sweet_ctr);
  while (men) {
    b1 = FirstBit(men);
    men &= sq_clr[b1];
    val -= 7-row(b1);
    if (mask_col[col(b1)] & men) val += 10;
  }
  //pieces
  men = w_knight | w_bish;
  while (men) {
    b1 = FirstBit(men);
    men &= sq_clr[b1];
    val += Center(b1);                        //credit centralization
  }
  men = b_knight | b_bish;
  while (men) {
    b1 = FirstBit(men);
    men &= sq_clr[b1];
    val -= Center(b1);
  }
  //if endgame move king to center
  if ((w_pieces + b_pieces) < 5) val += Center(wk_sq) - Center(bk_sq);
  //negate score if black to move
  if (color == BLACK) val = -val;
  return val;
}

