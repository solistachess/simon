//setboard.cpp Copyright (C) 2004 Dan Honeycutt.  All rights reserved.
#include "chess.h"

/************************************************************************
File contains SetBoard() which sets up the board and initializes data 
so simon is ready to play.  It calls ValidFen() which parses the fen 
string and places pieces on the board.  ValidFen() in turn calls 
ValidPos() which verifys that the position is legal.  if all is OK 
SetBoard() returns TRUE.  SetBoard() will not leave the board in a 
trashed state.  If there is a problem SetBoard() sets the board up to
the opening position.  If SetBoard() returns FALSE err_msg has a 
description of the problem.
************************************************************************/

//=======================================================================
//ValidPos() checks for such things as 3 kings, pawns on the first
//rank etc.  On entry pieces are on the board.  Returns TRUE if 
//position looks OK.
//=======================================================================
static bool ValidPos() {
  char c1;
  int b1, i1, n1;
  int nt[16] = {0};
  for (b1=0; b1<64; b1++) {
    c1 = (char) board[b1];
    if (!c1) continue;
    nt[c1]++;
    if (((c1 & TYPE) == PAWN) && ((row(b1)==0) || (row(b1)==7))) {
      strcpy(err_msg, "no pawns on first or last rank");
      return FALSE;
    }
  }
  if ((nt[WK] != 1) || (nt[BK] != 1)) {
    strcpy(err_msg, "each side must have exactly one king");
    return FALSE;
  }      
  for (i1=WHITE; i1<=BLACK; i1+=BLACK) {
    n1 = nt[PAWN+i1];
    if (nt[KNIGHT+i1] > 2) n1 += nt[KNIGHT+i1]-2;     //(excess) knights
    if (nt[BISHOP+i1] > 2) n1 += nt[BISHOP+i1]-2;     //bishops
    if (nt[ROOK+i1] > 2) n1 += nt[ROOK+i1]-2;         //rooks
    if (nt[QUEEN+i1] > 1) n1 += nt[QUEEN+i1]-1;       //queens
    if (n1 > 8) {
      strcpy(err_msg, "maximum 8 pawns plus promoted pieces");
      return FALSE;
    }
  }
  return TRUE;
}

//=======================================================================
//ValidFen() parses fen string and places pieces on board.  Sets 
//color, cast, ep, gp and move_num.  Returns TRUE if OK.
//=======================================================================
static bool ValidFen(char *fen) {
  char c1;
  int b1, flag=0;
  int i1 = 7;
  int j1 = 0;
  //clear board
  for (b1 = 0; b1 < 64; b1++) board[b1] = 0;
  //parse board position
  while (flag == 0) {
    c1 = *fen++;
    switch (c1) {
    case 0:
    case '\n':
      strcpy(err_msg, "incomplete fen");
      return FALSE;
    case ' ':
      flag = TRUE;
      break;
    case '/':
      i1--;
      j1 = 0;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
      j1 += c1 - '0';
      break;
    default:
      //piece - check rank/file before plopping it on board
      if ((i1 < 0) || (j1 > 7)) {
        strcpy(err_msg, "bad rank/file in fen");
        return FALSE;
      } else b1 = square(i1, j1);
      switch (c1) {
      case 'p':
        board[b1] = BP;
        break;
      case 'n':
        board[b1] = BN;
        break;
      case 'b':
        board[b1] = BB;
        break;
      case 'r':
        board[b1] = BR;
        break;
      case 'q':
        board[b1] = BQ;
        break;
      case 'k':
        board[b1] = BK;
        break;
      case 'P':
        board[b1] = WP;
        break;
      case 'N':
        board[b1] = WN;
        break;
      case 'B':
        board[b1] = WB;
        break;
      case 'R':
        board[b1] = WR;
        break;
      case 'Q':
        board[b1] = WQ;
        break;
      case 'K':
        board[b1] = WK;
        break;
      default:
        strcpy(err_msg, "invalid piece in fen");
        return FALSE;
      } //switch 2
      j1++;
    } //switch 1
  } //while flag
  while (isspace((int) *fen)) fen++;
  //color to move (PGN specs say lowercase but allow either)
  c1 = *fen++;
  switch (c1) {
  case 'w':
  case 'W':
    color = WHITE;
    break;
  case 'b':
  case 'B':
    color = BLACK;
    break;
  default:
    strcpy(err_msg, "bad color in fen");
    return FALSE;
  }
  //From here on info is not critical.  If string runs out or
  //strange chars encountered return ValidPos()
  move_num = 1;   //set defaults in case we exit early
  castle[0] = 0;
  ep_sq[0] = 0;
  g_ply[0] = 0;
  //castling rights
  while (isspace((int) *fen)) fen++;
  if (*fen != '-') {
    flag = 0;
    while (flag == 0) {
      c1 = *fen++;
      switch (c1) {
      case ' ':
        flag = TRUE;
        break;
      case 'K':
        if ((board[E1]==WK) && (board[H1]==WR)) castle[0] |= 1;
        break;
      case 'Q':
        if ((board[E1]==WK) && (board[A1]==WR)) castle[0] |= 2;
        break;
      case 'k':
        if ((board[E8]==BK) && (board[H8]==BR)) castle[0] |= 4;
        break;
      case 'q':
        if ((board[E8]==BK) && (board[A8]==BR)) castle[0] |= 8;
        break;
      default:
        //Garbage in the castling rights section!!  Assume kings
        //and rooks on their starting squares haven't moved.
        if ((board[E1]==WK) && (board[H1]==WR)) castle[0] |= 1;
        if ((board[E1]==WK) && (board[A1]==WR)) castle[0] |= 2;
        if ((board[E8]==BK) && (board[H8]==BR)) castle[0] |= 4;
        if ((board[E8]==BK) && (board[A8]==BR)) castle[0] |= 8;
        return ValidPos();
      }
    }
  } else  fen++;
  //ep square
  while (isspace((int) *fen)) fen++;
  if (*fen != '-') {
    j1 = 7 - 'h' + *fen++;
    if ((j1 < 0) || (j1 > 7)) return ValidPos();
    i1 = *fen++ - '1';
    if ((i1 < 0) || (i1 > 7)) return ValidPos();
    b1 = square(i1, j1);
    if (board[b1] == 0) {
      if (color) {
        if ((i1==2) && (board[b1+8]==WP) &&
          (((j1<7) && (board[b1+9]==BP)) ||
           ((j1>0) && (board[b1+7]==BP)))) ep_sq[0] = b1;
      } else {
        if ((i1==5) && (board[b1-8]==BP) &&
          (((j1>0) && (board[b1-9]==WP)) ||
           ((j1<7) && (board[b1-7]==WP)))) ep_sq[0] = b1;
      }
    }
  } else fen++;
  //halfmove clock
  while (isspace((int) *fen)) fen++;
  if (!isdigit(*fen)) return ValidPos();
  b1 = *fen++ - '0';
  while (isdigit(*fen)) {
    if (b1 < 100) {
      b1 = b1 * 10;
      b1 += *fen - '0';
    }
    fen++;
  }
  if (b1<100) g_ply[0] = b1;
  //move number
  while (isspace((int) *fen)) fen++;
  if (!isdigit(*fen)) return ValidPos();
  b1 = *fen++ - '0';
  while (isdigit(*fen)) {
    b1 = b1 * 10;
    b1 += *fen++ - '0';
    if (b1>1000) break;
  }
  move_num = b1;
  return ValidPos();
}

//====================================================================
//ValidEPSquare() checks to see if the ep pawn blocks an attack on 
//the king.  simon can play most positions that are technically 
//illegal but this one could cause a problem - making the ep capture
//exposes the king to capture.
//====================================================================
static bool ValidEPSquare() {
  U64 a1=0, enemy_bish, enemy_rook;
  int b1, dir, king;
  if (!ep_sq[0]) return TRUE;
  if (color) {   //black ep cap
    b1 = ep_sq[0] + 8;
    enemy_bish = w_bish | w_queen;
    enemy_rook = w_rook | w_queen;
    king = bk_sq;
  } else {
    b1 = ep_sq[0] - 8;
    enemy_bish = b_bish | b_queen;
    enemy_rook = b_rook | b_queen;
    king = wk_sq;
  }
  dir = directions[king][b1];
  if (dir && !(obstructed[king][b1] & (w_men|b_men))) {
    switch (abs_val(dir)) {
    case 1:
      a1 = atk_rank(b1) & enemy_rook;
      break;
    case 7:
      a1 = atk_rl45(b1) & enemy_bish;
      break;
    case 8:
      a1 = atk_file(b1) & enemy_rook;
      break;
    case 9:
      a1 = atk_rr45(b1) & enemy_bish;
      break;
    }
    if (a1) return FALSE;
  }
  return TRUE;
}

//====================================================================
//BBInit() initializes the bitboards, hash keys, etc.
//The following (basic board position) must already be set:
//color, board[0 to 63], castle[0], ep_sq[0], g_ply[0]
//====================================================================
static void BBInit() {
  int b1, c1;
  for (b1 = 0; b1<16; b1++) {
    bbd[b1] = 0;
    num_men[b1] = 0;
  }
  bb_rl90 = bb_rl45 = bb_rr45 = 0;
  key_1 = key_2 = 0;
  for (b1 = 0; b1 < 64; b1++) {
    if ((c1 = board[b1])) {
      board[b1] = 0;
      AddPiece(c1, b1);
      if (c1 == WK) wk_sq = b1;
      if (c1 == BK) bk_sq = b1;
    }
  }
  //remove kings from piece counts
  num_men[BISH_Q]--;
  num_men[ROOK_Q]--;
  //finish hash key
  key_1 ^= rnd_epc[castle[0]];
  key_1 ^= rnd_epc[ep_sq[0]];
  if (color) key_1 ^= rnd_btm;
}

//=======================================================================
//SetBoard() sets up the board ready for play.  If a bad fen string is
//passed SetBoard() returns FALSE (err_msg has a description of the 
//problem) and sets the board up to the starting position.
//=======================================================================
bool SetBoard(char *fen) {
  if (!ValidFen(fen)) {
    SetBoard();
    return FALSE;
  }
  kolor = (color ^ KTC);    //player gets to move first
  //finish bitboards so we can compute attacks and make
  //final validity checks
  BBInit();
  if (illegal) {
    SetBoard();
    strcpy(err_msg, "side not moving is in check");
    return FALSE;
  }
  if (!ValidEPSquare()) {
    SetBoard();
    strcpy(err_msg, "ep square gives illegal position");
    return FALSE;
  }
  //set game history
  hix = 0;
  hist[hix].key1 = key_1;
  hist[hix].cast = castle[0];
  hist[hix].ep = ep_sq[0];
  hist[hix].gp = g_ply[0];
  //generate root moves, verify there are some
  GenRoot();
  if (!root_moves) {
    SetBoard();
    strcpy(err_msg, "game over - no moves");
    return FALSE;
  }
  //looks OK.  Clear hash and history
  root_score = 0;
  game_over = 0;
  ClearHash();

  return TRUE;
}

