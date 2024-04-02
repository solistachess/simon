//chess.h by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.
#ifndef CHESS_H
#define CHESS_H

/*********************************************************************
Main header file
*********************************************************************/

//set debug_mode to 0 to turn off asserts
#define debug_mode 1
#if debug_mode
  #undef NDEBUG
#endif

//simon is supplied with no book but can use Bruja's book.
//You can email for the latest version if you wish to use it.
//#define bruja_book 0


#ifdef _MSC_VER
#define inline __inline
#define CDECL __cdecl
#else
#define CDECL
#include <ncurses.h>
#endif

static bool bruja_book    = false;


//====================================================================
//library includes
//====================================================================
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//====================================================================
//system specific
//====================================================================
#include "system.h"

//====================================================================
//constants
//====================================================================
#define FALSE         0
#define TRUE          1
#define INF           32767             //infinity
#define MATE          31000             //mate in n reference
#define DEAD          30000             //mate scores
#define MAX_PLY       64                //max search depth
#define MAX_HIX       200               //game history

#define WHITE         0
#define PAWN          1
#define KING          2
#define KNIGHT        3
#define BISHOP        5
#define ROOK          6
#define QUEEN         7
#define BLACK         8

#define ESQ           0     //empty square
#define NO_CASTLE     1     //(not) king/rook test bit
#define SLIDE         4     //sliding piece test bit
#define SLIDE_B       5     //mask - diagonal sliding piece
#define SLIDE_R       6     //mask - rank/file sliding piece
#define TYPE          7     //piece type mask
#define KTC           8     //color toggle/compare

//bitboard and piece count array index
typedef enum {
  W_MEN, WP, WK, WN, BISH_Q, WB, WR, WQ,
  B_MEN, BP, BK, BN, ROOK_Q, BB, BR, BQ
} bitboard_index;

//board squares
typedef enum {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8
} array_board_index;

//hash bounds
#define HF_LOWER  1   //lower bound
#define HF_UPPER  2   //upper bound
#define HF_EXACT  3   //exact score

//game_over constants
#define FIN_BLACK_MATED         1
#define FIN_WHITE_MATED         2
#define FIN_BLACK_RESIGNS       3
#define FIN_WHITE_RESIGNS       4
#define FIN_STALEMATE           5
#define FIN_3_REP               6
#define FIN_50_MOVE             7
#define FIN_LACK_FORCE          8

//====================================================================
//macros.
//====================================================================
#define row(bx) ((bx) >>3)                      //board row, 0 to 7
#define col(bx) ((bx) & 7)                      //board column, 0 to 7
#define square(row, col) (8*(row)+(col))        //board square 0 to 63
#define abs_val(v)      ((v) >= 0 ? (v) : -(v))

//extract move parts - short or long move
#define mv_b1(mv)       ((mv) & 63)             //from
#define mv_b2(mv)       (((mv)>>6) & 63)        //to
#define mv_pro(mv)      (((mv)>>12) & 7)        //promotion type
//long move only
#define mv_cap(mv)      (((mv)>>16) & 15)       //capture
#define mv_capro(mv)    ((mv) & 0xf7000)        //cap or pro
#define mv_man(mv)      (((mv)>>20) & 15)       //piece moving
#define mv_type(mv)     (((mv)>>20) & 7)        //piece type moving
#define mv_spl(mv)      (((mv)>>24) & 7)        //special move

//macros to save having to use index to bbd[] & num_men[]
#define w_pawn        bbd[WP]           //white pawn bitboard
#define b_pawn        bbd[BP]           //black pawn bitboard
#define w_knight      bbd[WN]           //etc
#define b_knight      bbd[BN]
#define w_bish        bbd[WB]
#define b_bish        bbd[BB]
#define w_rook        bbd[WR]
#define b_rook        bbd[BR]
#define w_queen       bbd[WQ]
#define b_queen       bbd[BQ]
#define w_king        bbd[WK]
#define b_king        bbd[BK]
#define w_men         bbd[W_MEN]        //all white men bitboard
#define b_men         bbd[B_MEN]        //all black men bitboard
#define bish_queen    bbd[BISH_Q]       //all bishops & queens
#define rook_queen    bbd[ROOK_Q]       //all rooks & queens

#define w_pawns       num_men[WP]       //number of white pawns
#define b_pawns       num_men[BP]       //number of black pawns
#define w_knights     num_men[WN]       //etc
#define b_knights     num_men[BN]
#define w_bishops     num_men[WB]
#define b_bishops     num_men[BB]
#define w_rooks       num_men[WR]
#define b_rooks       num_men[BR]
#define w_queens      num_men[WQ]
#define b_queens      num_men[BQ]
#define w_pieces      num_men[BISH_Q]   //white number of pieces
#define b_pieces      num_men[ROOK_Q]   //black number of pieces
#define white_mtl     num_men[W_MEN]    //white material
#define black_mtl     num_men[B_MEN]    //black material

#define occupied      (w_men|b_men)     //occupied squares
#define empty         (~occupied)       //empty squares

//attack macros
#define incheck (color?Attacked(bk_sq, WHITE):Attacked(wk_sq, BLACK))
#define illegal (color?Attacked(wk_sq, BLACK):Attacked(bk_sq, WHITE))

//individual piece attacks
#define atk_wpawn(sq)   (ap_wpawn[sq])
#define atk_bpawn(sq)   (ap_bpawn[sq])
#define atk_knight(sq)  (ap_knight[sq])
#define atk_rank(sq)    (ap_rook_rr00[sq][((occupied)>>((sq)&56))&127])
#define atk_file(sq)    (ap_rook_rl90[sq][(bb_rl90>>(col(sq)<<3))&127])
#define atk_rl45(sq)    (ap_bish_rl45[sq][(bb_rl45>>shr_bish_rl45[sq])&127])
#define atk_rr45(sq)    (ap_bish_rr45[sq][(bb_rr45>>shr_bish_rr45[sq])&127])
#define atk_rook(sq)    (atk_rank(sq)|atk_file(sq))
#define atk_bish(sq)    (atk_rl45(sq)|atk_rr45(sq))
#define atk_queen(sq)   (atk_rook(sq)|atk_bish(sq))
#define atk_king(sq)    (ap_king[sq])

//====================================================================
//structs
//====================================================================
//game history
typedef struct {
  U64           key1;   //hash key
  int           ep;     //en passant sq
  int           gp;     //half move clock
  int           cast;   //castle rights
  int           move;   //move to next position
} s_hist;

//move
typedef struct {
  int           move;   //move
  int           val;    //score
} s_move;

//search tree
typedef struct {
  s_move      * pm2;          //end of moves
  int           killer1;      //killer moves
  int           killer2;
  U64           key1;         //hash key
} s_tree;

//====================================================================
//simon includes
//====================================================================
#include "global.h"

#endif  //ifndef CHESS_H
