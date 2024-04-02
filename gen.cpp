//gen.cpp by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.
#include "chess.h"

/*********************************************************************
File contains the move generation functions GenCap(), GenMov() 
and GenEvade() along with associated support functions.

Two move formats are used - a "short" (2 byte) move which has
information necessary to look up the move in a list and a "long" (4 
byte) move which has information necessary to reverse the move.
bits (and shift values to extract parts) are:
  bits                  shift     note                extract macro
  --------------------  --------  -----               -------------
  6 bits - from         (>>0)     short & long move   mv_b1(mv)
  6 bits - to           (>>6)                         mv_b2(mv)
  3 bits - promotion    (>>12)                        mv_pro(mv)
  1 bit - not used
  4 bits - capture      (>>16)    long move only      mv_cap(mv)
  4 bits - piece moving (>>20)                        mv_man(mv)
  3 bits - spl move     (>>24)                        mv_spl(mv)
    1 (0x1000000 unshifted) = castle king side
    2 (0x2000000 unshifted) = castle queen side
    3 (0x3000000 unshifted) = PxP ep
    4 (0x4000000 unshifted) = Pawn promotion
    5 (0x5000000 unshifted) = Pawn 2 square advance
  1 bit - not used
  =====
  28 bits total = move saved in hash table.
*********************************************************************/

//squares that must be clear to castle
static const U64 mask_wck = 0x0000000000000060;
static const U64 mask_wcq = 0x000000000000000e;
static const U64 mask_bck = 0x6000000000000000;
static const U64 mask_bcq = 0x0e00000000000000;
//promotion bits
static const int queen_pro = 1;
static const int under_pro = 2;

//====================================================================
//SetPins() sets pins and pin_mask[] which tells us who is pinned and
//where they can go.  If a pin is found we set pin_mask[] to the 
//corresponding rank, file or diagonal from the king to and including
//the pinnor.
//====================================================================
static U64 SetPins() {
  int b1, b2, king;
  U64 enemy, a1, d1;      //pinor and pinee
  if (key_1 == pin_key1) {
    return pin_save;
  }
  pin_key1 = key_1;
  pin_save = 0;
  if (color) {        //set black pins
    king = bk_sq;
    enemy = w_men;
    d1 = b_men & atk_queen(king);
  } else {            //set white pins
    king = wk_sq;
    enemy = b_men;
    d1 = w_men & atk_queen(king);
  }
  while (d1) {
    b1 = FirstBit(d1);
    d1 &= sq_clr[b1];
    switch (abs_val(directions[king][b1])) {
    case 1:
      a1 = atk_rank(b1) & rook_queen & enemy;
      break;
    case 7:
      a1 = atk_rl45(b1) & bish_queen & enemy;
      break;
    case 8:
      a1 = atk_file(b1) & rook_queen & enemy;
      break;
    case 9:
      a1 = atk_rr45(b1) & bish_queen & enemy;
      break;
    }
    if (a1) {
      pin_save |= sq_set[b1];
      b2 = FirstBit(a1);
      pin_mask[b1] = a1 | obstructed[king][b2];
    }
  }
  return pin_save;
}

//====================================================================
//AddEPCap() generates en passant captures.
//target must be the en passant target square
//====================================================================
static s_move *AddEPCap(s_move *pm, int target, U64 pins) {
  U64 a1, men, enemy=0;
  int b1, b3, king, temp;

  if (color) {      //black ep cap
    men = b_pawn & ap_wpawn[target];
    b3 = target+8;
    king = bk_sq;
    if (row(b3) == row(king)) enemy = (w_rook | w_queen) & mask_row[row(king)];
    temp = (target<<6)|(BP<<20)|(WP<<16)|0x3000000;
  } else {        //white ep cap
    men = w_pawn & ap_bpawn[target];
    b3 = target-8;
    king = wk_sq;
    if (row(b3) == row(king)) enemy = (b_rook | b_queen) & mask_row[row(king)];
    temp = (target<<6)|(WP<<20)|(BP<<16)|0x3000000;
  }
  while (men) {
    b1 = FirstBit(men);
    men &= sq_clr[b1];
    a1 = sq_set[target];
    if (pins & sq_set[b1]) a1 &= pin_mask[b1];
    //make sure captured pawn is not sheltering an attack
    if (enemy && a1) {
      //king on pawn rank w/hostile rooks/queens
      if ((!(obstructed[b1][king] & occupied) && (atk_rank(b3) & enemy)) ||
          (!(obstructed[b3][king] & occupied) && (atk_rank(b1) & enemy))) {
        a1 = 0;
      }
    }
    if (a1) (pm++)->move = b1|temp;
  }
  return pm;
}

//====================================================================
//AddPawnPro() Generates pawn promotions.  Target can be all squares
//====================================================================
static s_move *AddPawnPro(s_move *pm, U64 target, U64 pins, int pro) {
  U64 men, moves[3];
  int i, delta, b1, b2, temp;

  if (color) { //black pawn promotions
    men = b_pawn & rank_2;
    if (!men) return pm;
    moves[0] = ((men & ~file_a) >> 9) & w_men & target;
    moves[1] = (men >> 8) & empty & target;
    moves[2] = ((men & ~file_h) >> 7) & w_men & target;
    delta = -9;
    temp = 0x4000000 | (BP<<20);
  } else { //white pawn promotions
    men = w_pawn & rank_7;
    if (!men) return pm;
    moves[0] = ((men & ~file_a) << 7) & b_men & target;
    moves[1] = (men << 8) & empty & target;
    moves[2] = ((men & ~file_h) << 9) & b_men & target;
    delta = 7;
    temp = 0x4000000 | (WP<<20);
  }  
  for (i=0; i<3; i++) {
    while (moves[i]) {
      b2 = FirstBit(moves[i]);
      moves[i] &= sq_clr[b2];
      b1 = b2 - delta;
      if ((!(pins & sq_set[b1])) || (sq_set[b2] & pin_mask[b1])) {
        if (pro & queen_pro) {
          (pm++)->move = b1|(b2 << 6)|temp|(board[b2]<<16)|(QUEEN << 12);
        }
        if (pro & under_pro) {
          (pm++)->move = b1|(b2 << 6)|temp|(board[b2]<<16)|(KNIGHT << 12);
          (pm++)->move = b1|(b2 << 6)|temp|(board[b2]<<16)|(BISHOP << 12);
          (pm++)->move = b1|(b2 << 6)|temp|(board[b2]<<16)|(ROOK << 12);
        }
      }
    } //while moves
    delta++;
  } //for i      
  return pm;
}

//====================================================================
//AddXPawnCap() generates non promotion pawn captures (except ep caps)
//Target can be all squares.
//====================================================================
static s_move *AddBPawnCap(s_move *pm, U64 target, U64 pins) {
  int b1, b2;
  U64 men, moves1, moves2;
  men = b_pawn & (~rank_2);
  target &= w_men;
  moves1 = ((men & ~file_a) >> 9) & target;
  moves2 = ((men & ~file_h) >> 7) & target;
  if (pins) { //pawn cap - pins
    while (moves1) {
      b2 = FirstBit(moves1);
      moves1 &= sq_clr[b2];
      b1 = b2 + 9;
      if ((!(pins & sq_set[b1])) || (sq_set[b2] & pin_mask[b1])) {
        (pm++)->move = b1|(b2<<6)|(BP << 20)|(board[b2]<<16);
      }
    }
    while (moves2) {
      b2 = FirstBit(moves2);
      moves2 &= sq_clr[b2];
      b1 = b2 + 7;
      if ((!(pins & sq_set[b1])) || (sq_set[b2] & pin_mask[b1])) {
        (pm++)->move = b1|(b2<<6)|(BP<<20)|(board[b2]<<16);
      }
    }
  } else { //pawn cap - no pins
    while (moves1) {
      b2 = FirstBit(moves1);
      moves1 &= sq_clr[b2];
      (pm++)->move = (b2+9)|(b2<<6)|(BP<<20)|(board[b2]<<16);
    }
    while (moves2) {
      b2 = FirstBit(moves2);
      moves2 &= sq_clr[b2];
      (pm++)->move = (b2+7)|(b2<<6)|(BP<<20)|(board[b2]<<16);
    }
  } //if pins
  return pm;
} //AddBPawnCap()

static s_move *AddWPawnCap(s_move *pm, U64 target, U64 pins) {
  int b1, b2;
  U64 men, moves1, moves2;
  men = w_pawn & (~rank_7);
  target &= b_men;
  moves1 = ((men & ~file_a) << 7) & target;
  moves2 = ((men & ~file_h) << 9) & target;
  if (pins) { //pawn cap - pins
    while (moves1) {
      b2 = FirstBit(moves1);
      moves1 &= sq_clr[b2];
      b1 = b2 - 7;
      if ((!(pins & sq_set[b1])) || (sq_set[b2] & pin_mask[b1])) {
        (pm++)->move = b1|(b2<<6)|(WP<<20)|(board[b2]<<16);
      }
    }
    while (moves2) {
      b2 = FirstBit(moves2);
      moves2 &= sq_clr[b2];
      b1 = b2 - 9;
      if ((!(pins & sq_set[b1])) || (sq_set[b2] & pin_mask[b1])) {
        (pm++)->move = b1|(b2<<6)|(WP<<20)|(board[b2]<<16);
      }
    }
  } else { //pawn cap - no pins
    while (moves1) {
      b2 = FirstBit(moves1);
      moves1 &= sq_clr[b2];
      (pm++)->move = (b2-7)|(b2<<6)|(WP<<20)|(board[b2]<<16);
    }
    while (moves2) {
      b2 = FirstBit(moves2);
      moves2 &= sq_clr[b2];
      (pm++)->move = (b2-9)|(b2<<6)|(WP<<20)|(board[b2]<<16);
    }
  } //if pins
  return pm;
} //AddWPawnCap()

//====================================================================
//AddXPawnAdv() generates pawn non capture moves.
//Target can be all squares.
//====================================================================
static s_move *AddBPawnAdv(s_move *pm, U64 target, U64 pins) {
  int b1, b2;
  U64 men, moves1, moves2;
  men = b_pawn & ~rank_2;
  moves1 = (men >> 8) & empty;
  moves2 = ((moves1 & rank_6) >> 8) & empty & target;
  moves1 &= target;
  if (pins) { //pawn mov - pins
    while (moves1) {
      b2 = FirstBit(moves1);
      moves1 &= sq_clr[b2];
      b1 = b2 + 8;
      if ((!(pins & sq_set[b1])) || (sq_set[b2] & pin_mask[b1])) {
        (pm++)->move = b1|(b2<<6)|(BP<<20);
      }
    }
    while (moves2) {
      b2 = FirstBit(moves2);
      moves2 &= sq_clr[b2];
      b1 = b2 + 16;
      if ((!(pins & sq_set[b1])) || (sq_set[b2] & pin_mask[b1])) {
        (pm++)->move = b1|(b2<<6)|((BP<<20)|0x5000000);
      }
    }
  } else { //pawn mov - no pins
    while (moves1) {
      b2 = FirstBit(moves1);
      moves1 &= sq_clr[b2];
      (pm++)->move = (b2 + 8)|(b2<<6)|(BP<<20);
    }
    while (moves2) {
      b2 = FirstBit(moves2);
      moves2 &= sq_clr[b2];
      (pm++)->move = (b2 + 16)|(b2<<6)|((BP<<20)|0x5000000);
    }
  } //if pins
  return pm;
} //AddBPawnAdv()

static s_move *AddWPawnAdv(s_move *pm, U64 target, U64 pins) {
  int b1, b2;
  U64 men, moves1, moves2;
  men = w_pawn & ~rank_7;
  moves1 = (men << 8) & empty;
  moves2 = ((moves1 & rank_3) << 8) & empty & target;
  moves1 &= target;
  if (pins) { //pawn mov - pins
    while (moves1) {
      b2 = FirstBit(moves1);
      moves1 &= sq_clr[b2];
      b1 = b2 - 8;
      if ((!(pins & sq_set[b1])) || (sq_set[b2] & pin_mask[b1])) {
        (pm++)->move = b1|(b2<<6)|(WP<<20);
      }
    }
    while (moves2) {
      b2 = FirstBit(moves2);
      moves2 &= sq_clr[b2];
      b1 = b2 - 16;
      if ((!(pins & sq_set[b1])) || (sq_set[b2] & pin_mask[b1])) {
        (pm++)->move = b1|(b2<<6)|((WP<<20)|0x5000000);
      }
    }
  } else { //pawn mov - no pins
    while (moves1) {
      b2 = FirstBit(moves1);
      moves1 &= sq_clr[b2];
      (pm++)->move = (b2 - 8)|(b2<<6)|(WP<<20);
    }
    while (moves2) {
      b2 = FirstBit(moves2);
      moves2 &= sq_clr[b2];
      (pm++)->move = (b2 - 16)|(b2<<6)|((WP<<20)|0x5000000);
    }
  } //if pins
  return pm;
} //AddWPawnAdv()

//====================================================================
//AddPieceMov() generates piece capture & non capture moves.
//target must have correct target squares.
//====================================================================
static s_move *AddPieceMov(s_move *pm, U64 target, U64 &pins, U64 men) {
  int b1, b2, c1, temp;
  U64 moves;
  assert((men & (w_pawn | b_pawn))==0);
  //first do any pinned pieces
  while (men & pins) {
    b1 = FirstBit(men & pins);
    men &= sq_clr[b1];
    pins &= sq_clr[b1];
    c1 = board[b1];
    switch (c1 & TYPE) {
    case KNIGHT:
      continue;
    case BISHOP:
      moves = atk_bish(b1) & target & pin_mask[b1];
      break;
    case ROOK:
      moves = atk_rook(b1) & target & pin_mask[b1];
      break;
    case QUEEN:
      moves = atk_queen(b1) & target & pin_mask[b1];
      break;
    case KING:
      assert(0);   //uh oh
    } //switch piece moving
    temp = b1 + (c1 << 20);
    while (moves) {
      b2 = FirstBit(moves);
      moves &= sq_clr[b2];
      (pm++)->move = temp | (b2 << 6) | (board[b2] << 16);
    }
  } //while pinned men
  //now the rest of the pieces
  while (men) {
    b1 = FirstBit(men);
    men &= sq_clr[b1];
    c1 = board[b1];
    temp = b1 + (c1 << 20);
    switch (c1 & TYPE) {
    case KNIGHT:
      moves = atk_knight(b1) & target;
      break;
    case BISHOP:
      moves = atk_bish(b1) & target;
      break;
    case ROOK:
      moves = atk_rook(b1) & target;
      break;
    case QUEEN:
      moves = atk_queen(b1) & target;
      break;
    case KING:
      moves = atk_king(b1) & target;
      c1 = color ^ KTC;    //enemy color
      while (moves) {
        b2 = FirstBit(moves);
        moves &= sq_clr[b2];
        if (!Attacked(b2, c1)) {
          (pm++)->move = temp | (b2 << 6) | (board[b2] << 16);
        }
      }
      continue;
    } //switch piece moving
    while (moves) {
      b2 = FirstBit(moves);
      moves &= sq_clr[b2];
      (pm++)->move = temp | (b2 << 6) | (board[b2] << 16);
    }
  } //while men
  return pm;
}

//====================================================================
//GenCap() generates captures & queen promotions
//====================================================================
s_move *GenCap(s_move *pm, int ply) {
  U64 pins = SetPins();
  //we do piece moves first since they can clear pins
  if (color) {
    pm = AddPieceMov(pm, w_men, pins, b_men & ~b_pawn);
    pm = AddBPawnCap(pm, all_64, pins);
  } else {
    pm = AddPieceMov(pm, b_men, pins, w_men & ~w_pawn);
    pm = AddWPawnCap(pm, all_64, pins);
  }
  if (ep_sq[ply]) pm = AddEPCap(pm, ep_sq[ply], pins);
  return AddPawnPro(pm, all_64, pins, queen_pro);
}

//====================================================================
//GenMov() generates non captures & under promotions
//====================================================================
s_move *GenMov(s_move *pm, int ply) {
  U64 pins = SetPins();
  if (color) {      //black moves
    //black castle
    if ((castle[ply] & 4) && !(occupied & mask_bck) &&
      !Attacked(F8, WHITE) && !Attacked(G8, WHITE)) {
      (pm++)->move = 27267004;
    }
    if ((castle[ply] & 8) && !(occupied & mask_bcq) &&
      !Attacked(C8, WHITE) && !Attacked(D8, WHITE)) {
      (pm++)->move = 44043964;
    }
    //black moves
    pm = AddPieceMov(pm, empty, pins, b_men & ~b_pawn);
    pm = AddBPawnAdv(pm, all_64, pins);
  } else {        //white moves
    //white castle
    if ((castle[ply] & 1) && !(occupied & mask_wck) &&
      !Attacked(F1, BLACK) && !Attacked(G1, BLACK)) {
      (pm++)->move = 18874756;
    }
    if ((castle[ply] & 2) && !(occupied & mask_wcq) &&
      !Attacked(C1, BLACK) && !Attacked(D1, BLACK)) {
      (pm++)->move = 35651716;
    }
    //white moves
    pm = AddPieceMov(pm, empty, pins, w_men & ~w_pawn);
    pm = AddWPawnAdv(pm, all_64, pins);
  }
  return AddPawnPro(pm, all_64, pins, under_pro);
}

//====================================================================
//GenEvade() generates check evasions
//====================================================================
s_move *GenEvade(s_move *pm, int ply) {
  U64 target = 0, pins, moves, attacks;
  int b1, b2, temp, num;
  int dir1 = 0, dir2 = 0;

  if (color) {    //black in check
    //first, see who is giving check and where they reside
    b1 = bk_sq;
    attacks = Attacks(b1) & w_men;
    num = BitCount(attacks);
    b2 = FirstBit(attacks);
    if (num == 1) {
      dir1 = directions[b2][b1];
      if (dir1) { //dir1 is 0 if the checking piece is a knight
        target = obstructed[b1][b2];
        //unlike a bishop or rook we can run away from a pawn in
        //the opposite direction
        if (board[b2] == WP) dir1 = 0;
      }
      target |= attacks;
    } else {
      if (board[b2] != WP) dir1 = directions[b2][b1];
      attacks &= sq_clr[b2];
      b2 = FirstBit(attacks);
      if (board[b2] != WP) dir2 = directions[b2][b1];
    }
    //black king evasions.  look for squares not attacked and squares
    //not in the opposite direction of attack
    temp = b1 + (BK << 20);
    moves = ap_king[b1] & ~b_men;
    while (moves) {
      b2 = FirstBit(moves);
      moves &= sq_clr[b2];
      if (!Attacked(b2, WHITE) && (directions[b1][b2] != dir1)
        && (directions[b1][b2] != dir2)) {
        (pm++)->move = temp | (b2 << 6) | (board[b2] << 16);
      }
    }
    //if there are multiple attackors we are done as the only way
    //out of check is move the king.
    if (num > 1) return (pm);
    //only one attackor - look for pieces that can capture the
    //offender or block the attack
    pins = SetPins();
    pm = AddPieceMov(pm, target, pins, b_men & ~(b_pawn|b_king));
    pm = AddBPawnAdv(pm, target, pins);
    pm = AddBPawnCap(pm, target, pins);
    //if there is an ep capture possible we look to see if (1) putting
    //our pawn on the ep_square will intervine (? possible) and (2) if
    //that's the little bugger checking us.
    if (ep_sq[ply] && ((sq_set[ep_sq[ply]] & target) || 
      (b_king & ap_wpawn[ep_sq[ply]+8])))
      pm = AddEPCap(pm, ep_sq[ply], pins);
  } else {        //white in check
    //first, see who is giving check and where they reside
    b1 = wk_sq;
    attacks = Attacks(b1) & b_men;
    num = BitCount(attacks);
    b2 = FirstBit(attacks);
    if (num == 1) {
      dir1 = directions[b2][b1];
      if (dir1) { //dir1 is 0 if the checking piece is a knight
        target = obstructed[b1][b2];
        //unlike a bishop or rook we can run away from a pawn in
        //the opposite direction
        if (board[b2] == BP) dir1 = 0;
      }
      target |= attacks;
    } else {
      if (board[b2] != BP) dir1 = directions[b2][b1];
      attacks &= sq_clr[b2];
      b2 = FirstBit(attacks);
      if (board[b2] != BP) dir2 = directions[b2][b1];
    }
    //white king evasions.  look for squares not attacked and squares
    //not in the opposite direction of attack
    temp = b1 + (WK << 20);
    moves = ap_king[b1] & ~w_men;
    while (moves) {
      b2 = FirstBit(moves);
      moves &= sq_clr[b2];
      if (!Attacked(b2, BLACK) && (directions[b1][b2] != dir1)
        && (directions[b1][b2] != dir2)) {
        (pm++)->move = temp | (b2 << 6) | (board[b2] << 16);
      }
    }
    //if there are multiple attackors we are done as the only way
    //out of check is move the king.
    if (num > 1) return (pm);
    //only one attackor - look for pieces that can capture the
    //offender or block the attack
    pins = SetPins();
    pm = AddPieceMov(pm, target, pins, w_men & ~(w_pawn|w_king));
    pm = AddWPawnAdv(pm, target, pins);
    pm = AddWPawnCap(pm, target, pins);
    //if there is an ep capture possible we look to see if (1) putting
    //our pawn on the ep_square will intervine (? possible) and (2) if
    //that's the little bugger checking us.
    if (ep_sq[ply] && ((sq_set[ep_sq[ply]] & target) || 
      (w_king & ap_bpawn[ep_sq[ply]-8])))
      pm = AddEPCap(pm, ep_sq[ply], pins);
  } //if color
  return AddPawnPro(pm, target, pins, queen_pro|under_pro);
}

//====================================================================
//GenRoot() generates root moves, sets root_moves. Called from
//SetBoard() to initialize and by MakeMove() to keep current.
//====================================================================
void GenRoot(void) {
  s_move *pm = root_list;
  if (incheck) pm = GenEvade(pm, 0);
  else pm = GenMov(GenCap(pm, 0), 0);
  root_moves = pm - root_list;
}


