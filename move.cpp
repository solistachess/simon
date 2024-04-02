//move.cpp by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.
#include "chess.h"

/*********************************************************************
File includes functions to make and unmake moves.
*********************************************************************/

//====================================================================
//AddPiece() places a piece on the board
//====================================================================
void AddPiece(int c1, int b1) {
  assert(board[b1]==ESQ);
  bb_move = sq_set[b1];
  board[b1] = c1;               //place on array board
  bbd[c1] |= bb_move;           //place on piece bitboard
  bbd[c1 & KTC] |= bb_move;     //place on all men bitboard
  bb_rl90 |= sq_set_rl90[b1];   //place on the 3 rotated boards
  bb_rl45 |= sq_set_rl45[b1];
  bb_rr45 |= sq_set_rr45[b1];
  key_1 ^= rnd_psq[c1][b1];     //update hash key
  num_men[c1]++;                //update count of piece
  num_men[c1 & KTC] += piece_value[c1];   //total material (B & W)
  //simon doesn't use a pawn hash key but it maintains one for
  //good measure.  You may find it useful.
  if ((c1 & TYPE) == PAWN) key_2 ^= rnd_psq[c1][b1];
  else {
    num_men[(c1 & KTC) + BISH_Q]++;       //total pieces (B & W)
    if (c1 & SLIDE) {                     //update sliding pieces
      if ((c1 & SLIDE_B)==SLIDE_B) bbd[BISH_Q] |= bb_move;
      if ((c1 & SLIDE_R)==SLIDE_R) bbd[ROOK_Q] |= bb_move;
    }
  }    
} //AddPiece()

//====================================================================
//MovePiece() relocates a piece.
//====================================================================
void MovePiece(int c1, int b1, int b2) {
  assert(board[b1]==c1 && board[b2]==ESQ);
  bb_move = sq_set[b1] | sq_set[b2];
  board[b1] = ESQ;
  board[b2] = c1;
  bbd[c1] ^= bb_move;
  bbd[c1 & KTC] ^= bb_move;
  bb_rl90 ^= sq_set_rl90[b1] | sq_set_rl90[b2];
  bb_rl45 ^= sq_set_rl45[b1] | sq_set_rl45[b2];
  bb_rr45 ^= sq_set_rr45[b1] | sq_set_rr45[b2];
  key_1 ^= rnd_psq[c1][b1];
  key_1 ^= rnd_psq[c1][b2];
  if (c1 & SLIDE) {
    if ((c1 & SLIDE_B)==SLIDE_B) bbd[BISH_Q] ^= bb_move;
    if ((c1 & SLIDE_R)==SLIDE_R) bbd[ROOK_Q] ^= bb_move;
  } else if ((c1 & TYPE) == PAWN) {
    key_2 ^= rnd_psq[c1][b1];
    key_2 ^= rnd_psq[c1][b2];
  }
} //MovePiece()

//====================================================================
//RemovePiece() removes a piece.
//====================================================================
void RemovePiece(int c1, int b2) {
  assert((board[b2]==c1) && ((c1 & TYPE) != KING));
  bb_move = sq_set[b2];
  board[b2] = ESQ;
  bbd[c1] ^= bb_move;
  bbd[c1 & KTC] ^= bb_move;
  bb_rl90 ^= sq_set_rl90[b2];
  bb_rl45 ^= sq_set_rl45[b2];
  bb_rr45 ^= sq_set_rr45[b2];
  key_1 ^= rnd_psq[c1][b2];
  num_men[c1]--;
  num_men[c1 & KTC] -= piece_value[c1];
  //piece specific
  if ((c1 & TYPE) == PAWN) key_2 ^= rnd_psq[c1][b2];
  else {
    num_men[(c1 & KTC) + BISH_Q]--;   //piece count
    if (c1 & SLIDE) {
      if ((c1 & SLIDE_B)==SLIDE_B) bbd[BISH_Q] ^= bb_move;
      if ((c1 & SLIDE_R)==SLIDE_R) bbd[ROOK_Q] ^= bb_move;    
    }
  }
} //RemovePiece()
  
//====================================================================
//Move() makes a move and updates position data.  It also updates
//en passant square, castling status, game ply and hash keys.
//====================================================================
void Move(int move, int ply) {
  int b1, b2, man, cap, temp;
  castle[ply+1] = castle[ply];      //copy castling rights
  ep_sq[ply+1] = 0;                 //clear ep square
  key_1 ^= rnd_epc[ep_sq[ply]];     //(set later if a 2 square advance)
  g_ply[ply+1] = g_ply[ply] + 1;    //increment half move clock
  man = mv_man(move);               //piece moving
  b1 = mv_b1(move);                 //from
  b2 = mv_b2(move);                 //to
  cap = mv_cap(move);               //capture
  //if this is a capture or pawn move reset the half move clock
  if (cap || ((man & TYPE)==PAWN)) g_ply[ply+1] = 0;
  //check for a king/rook move or a rook capture
  if ((!(man & NO_CASTLE)) || ((cap & TYPE) == ROOK)) {
    switch (man) {
    case WK:
      castle[ply+1] &= 12;
      wk_sq = b2;
      break;
    case WR:
      if (b1 == A1) castle[ply+1] &= 13;
      else if (b1 == H1) castle[ply+1] &= 14;
      break;
    case BK:
      castle[ply+1] &= 3;
      bk_sq = b2;
      break;
    case BR:
      if (b1 == A8) castle[ply+1] &= 7;
      else if (b1 == H8) castle[ply+1] &= 11;
      break;
    }
    switch (cap) {
    case WR:
      if (b2 == A1) castle[ply+1] &= 13;
      else if (b2 == H1) castle[ply+1] &= 14;
      break;
    case BR:
      if (b2 == A8) castle[ply+1] &= 7;
      else if (b2 == H8) castle[ply+1] &= 11;
      break;
    }
    key_1 ^= rnd_epc[castle[ply]];
    key_1 ^= rnd_epc[castle[ply+1]];
  }
  //special move handling
  switch (mv_spl(move)) {
  case 1:   //O-O
    MovePiece(board[b1+3], b1+3, b1+1);     //move the rook
    g_ply[ply+1] = 0;                               //non reversible
    break;
  case 2:   //O-O-O
    MovePiece(board[b1-4], b1-4, b1-1);
    g_ply[ply+1] = 0;
    break;
  case 3:   //PxP ep
    RemovePiece(cap, color ? b2+8 : b2-8);  //make capture
    cap = 0;
    break;
  case 4:   //Pawn promotion
    RemovePiece(man, b1);                   //remove pawn
    man = mv_pro(move) + color;
    AddPiece(man, b1);                //replace w/promotion
    break;
  case 5:   //pawn 2 square advance
    if (color) {
      temp = b2+8;                            //square behind
      if (ap_bpawn[temp] & bbd[WP]) {         //attack by enemy pawn
        ep_sq[ply+1] = temp;                  //yes - set ep square
        key_1 ^= rnd_epc[temp];
      }
    } else {
      temp = b2-8;
      if (ap_wpawn[temp] & bbd[BP]) {
        ep_sq[ply+1] = temp;
        key_1 ^= rnd_epc[temp];
      }
    }
    break;
  } //switch spl
  if (cap) RemovePiece(cap, b2);        //remove cap if any
  MovePiece(man, b1, b2);               //make the move
  color ^= KTC;                                 //toggle color to move
  key_1 ^= rnd_btm;
}

//====================================================================
//UnMove() reverses a move.  The inverse of Move()
//====================================================================
void UnMove(int move, int ply) {
  int b1, b2, man, cap;
  color ^= KTC;                     //toggle color to move
  key_1 ^= rnd_btm;
  key_1 ^= rnd_epc[ep_sq[ply+1]];   //restore ep square
  key_1 ^= rnd_epc[ep_sq[ply]];
  man = mv_man(move);               //piece moving
  b1 = mv_b1(move);                 //from
  b2 = mv_b2(move);                 //to
  cap = mv_cap(move);               //capture
  //check for king/rook move or rook capture
  if ((!(man & NO_CASTLE)) || ((cap & TYPE) == ROOK)) {
    if ((man & TYPE) == KING) {
      if (man & BLACK) bk_sq = b1;
      else wk_sq = b1;
    }
    key_1 ^= rnd_epc[castle[ply]];
    key_1 ^= rnd_epc[castle[ply+1]];
  }    
  //special move handling
  switch (mv_spl(move)) {
  case 1:   //O-O
    MovePiece(board[b1+1], b1+1, b1+3);   //move the rook
    break;
  case 2:   //O-O-O
    MovePiece(board[b1-1], b1-1, b1-4);
    break;
  case 3:   //PxP ep
    AddPiece(cap, color ? b2+8 : b2-8);   //replace capture
    cap = 0;
    break;
  case 4:   //Pawn promotion
    RemovePiece(mv_pro(move)+color, b2);  //remove piece
    AddPiece(man, b2);                    //replace w/pawn
    break;
  } //switch spl
  MovePiece(man, b2, b1);                 //make the move
  if (cap) AddPiece(cap, b2);             //replace cap if any
}

//====================================================================
//MoveNull() makes a null move.  simon does not employ a null move
//but if you add one this will correctly make the null move
//====================================================================
void MoveNull(int ply) {
  castle[ply+1] = castle[ply];        //copy castling rights
  ep_sq[ply+1] = 0;                   //clear ep square
  key_1 ^= rnd_epc[ep_sq[ply]];
  g_ply[ply+1] = g_ply[ply] + 1;      //increment half move clock
  color ^= KTC;                       //toggle color to move
  key_1 ^= rnd_btm;
}

//====================================================================
//UnMove() reverses a null move.  The inverse of MoveNull()
//====================================================================
void UnMoveNull(int ply) {
  color ^= KTC;                         //toggle color to move
  key_1 ^= rnd_btm;
  key_1 ^= rnd_epc[ep_sq[ply]];         //restore ep square
}

//====================================================================
//MakeMove() plays a move in the game and updates game history.  It 
//returns FIN_XXX if the move ends the game.
//====================================================================
int MakeMove(int move) {
  int i, discard = 20;
  Move(move, 0);
  hist[hix].move = move;
  hix++;
  hist[hix].key1 = key_1;
  hist[hix].cast = castle[0] = castle[1];
  hist[hix].ep = ep_sq[0] = ep_sq[1];
  hist[hix].gp = g_ply[0] = g_ply[1];
  //check to see if history is full - if so throw some away
  if (hix >= MAX_HIX) {
    for (i=0; i <= hix-discard; i++) {
      hist[i] = hist[i + discard];
    }
    hix -= discard;
  }
  if (color == WHITE) move_num++;
  //game is in progress since a move was made.  check for game end.
  //check mate first - if the 50th move produces mate it's not a draw
  GenRoot();
  if (!root_moves) {
    if (incheck) {
      return (color ? FIN_BLACK_MATED : FIN_WHITE_MATED);
    } else {
      return FIN_STALEMATE;
    }
  }
  if (g_ply[0] >= 100) return FIN_50_MOVE;
  if (Draw3Rep(0, FALSE)) return FIN_3_REP;
  if (!CanWin()) return FIN_LACK_FORCE;
  
  return 0;
}

//====================================================================
//UnMakeMove() will undo one move.  Inverse of MakeMove().  It 
//returns FALSE if there is not enough history to unmake the move.
//====================================================================
int UnMakeMove() {
  int move;
  if (hix == 0) return FALSE;
  hix--;
  move = hist[hix].move;
  castle[1] = castle[0];
  castle[0] = hist[hix].cast;
  ep_sq[1] = ep_sq[0];
  ep_sq[0] = hist[hix].ep;
  g_ply[1] = g_ply[0];
  g_ply[0] = hist[hix].gp;
  UnMove(move, 0);
  if (color == BLACK) move_num--;
  GenRoot();
  return (TRUE);
}

