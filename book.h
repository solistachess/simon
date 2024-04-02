//book.h.  Copyright (C) 2004 Dan Honeycutt.  You are free
//to use this software.  There is no warranty of any kind.

#ifndef BOOK_H
#define BOOK_H

/*********************************************************************
The book class is used to select a move from the book by the following
method:

const char * CBook::GetBookMove(const CBookPos *)

To use the book class the engine needs 1 CBook variable, 1
CBookPos variable and two translation functions.  The first
translation routine converts the engine board representation
to a form that CBook understands, a CBookPos.  The second
translation function converts the string returned by
GetBookMove() to a form the engine understands.

CBookPos has three member variables:

  char bk_board[64];
  char bk_color;
  char bk_ep_target;
 
bk_board[0] = square a1
bk_board[1] = square b1
:
bk_board[63] = square h8

contents of bk_board[]:
0 = empty square
1 = white pawn
2 = white knight
3 = white bishop
4 = white rook
5 = white queen
6 = white king
7 = white rook with rights to castle
8 = not used
9 = black pawn
10 = black knight
:
15 = black rook with rights to castle

bk_color is color to move, 0 if white, non-zero if black.
bk_ep_target is enpassant target square, 16 to 47 or 0 if none.

if GetBookMove() fails to find a book move it returns a null
string.  If it finds a move it returns either 4 or 5 characters
in the form e2e4 or d7d8Q.  This is the same format used by 
Winboard so if the engine is Winboard compatable it should already
have the second translation function.
*********************************************************************/

//CBookPos - see above
typedef struct {
  char bk_board[64];          //board array
  char bk_color;              //color moving
  char bk_ep_target;          //en passant sq
} CBookPos;                       

//maximum number of moves for a book position
#define BK_MAX_MOVES  32

//bitboard data type
#ifdef WIN32
typedef unsigned __int64 bk_bbd;
#else
typedef unsigned long long bk_bbd;
#endif

//CBookDef - default book moves
typedef struct {
  bk_bbd    bk_key;                     //Hash key
  char      bk_value[BK_MAX_MOVES];     //move weight percent
  short     bk_moves[BK_MAX_MOVES];     //moves
} CBookDef;                       

//CBookMov - same as default w/statistics for position
typedef struct {
  bk_bbd    bk_key;                     //Hash key
  short     bk_win;                     //stats (not used by CBook)
  short     bk_loss;
  short     bk_draw;
  char      bk_value[BK_MAX_MOVES];     //move weight percent
  short     bk_moves[BK_MAX_MOVES];     //moves
} CBookMov;                       
                                
//CBook
class CBook {
  int       bk_num_pos;         //number of positions in the book
  int       bk_min_men;         //minimum number men to be in book
  int       bk_using_default;   //flag - default book in use
  bk_bbd    (*bk_rnd)[64];      //bk_rnd[16][64] - random hash values

  int       SelectBookMove(const CBookPos *cur_pos);
  int       BinSearch(bk_bbd key);
  int       Flip(int sq);
  bk_bbd    BookEntryKey(int ix);
  void      CopyBookEntry(int ix);
  void      InitBook(void);

public:
  //returns book move in coordinate format
  const char *GetBookMove(const CBookPos *cur_pos);

  //Look for a book entry
  bool      FindBookEntry(const CBookPos *cur_pos);
  //if entry found following are set
  CBookMov  bk_entry;           //book entry if one is found
  int       bk_num_moves;       //number of moves for bk_entry
  int       bk_color_rev;       //color reversal flag

  //book hash key - for validity check
  bk_bbd    GetHashKey(const CBookPos *p1);

  CBook() {InitBook();}
};

#endif  //BOOK_H
