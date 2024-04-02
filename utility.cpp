//utility.cpp by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.
#include "chess.h"

/*********************************************************************
File contains miscellaneous utility functions
*********************************************************************/

//====================================================================
//Print() is printf() with a nl appended and an fflush() following.
//====================================================================
void Print(const char *fmt, ...) {
  char o_buf[512];
  va_list	ap;
  va_start(ap, fmt);
  vsprintf(o_buf, fmt, ap);
  printf("%s\n", o_buf);
	fflush(stdout);
}

//====================================================================
//Len() is simply strlen().  i have gotten some mystery results using
//strlen() inside a loop construct.
//====================================================================
int Len(const char *pc) {
  int i = 0;
  while (*pc++) i++;
  return i;
}

//====================================================================
//Val() is equivalent to atoi()  returns numeric value of string
//====================================================================
int Val(const char *pc) {
  int k1 = 0;
  long rv = 0;
  while (isspace((int) *pc)) pc++;
  if (*pc == '-') {
    k1 = TRUE;
    pc++;
  }
  while (isdigit((int) *pc)) {
    rv = rv*10;
    rv += *pc - '0';
    pc++;
  }
  if (k1) rv = -rv;
  return rv;
}

//====================================================================
//UpdateHistory() updates history heuristic and killer moves
//====================================================================
void UpdateHistory(int move, int ply, int depth) {
  int i1;
  unsigned *hh;
  if (!mv_capro(move)) {  //only non-captures go in history heuristic
    i1 = move & 4095;
    hh = color ? hh_black : hh_white;
    hh[i1] += depth * depth;
    if (hh[i1] > hh_max) hh_max = hh[i1];
    //update killers
    if (tree[ply].killer1 != move) {
      tree[ply].killer2 = tree[ply].killer1;
      tree[ply].killer1 = move;
    }
  }
}


//====================================================================
//MateIn() converts ply based mate scores to number of moves:
//  0 = No mate
//  1 = mate in 1  (30999 w/DEAD = 31000)
// -1 = mated in 1 (-30998)
//  2 = mate in 2  (30997) etc.
//====================================================================
int MateIn(int val) {
  if (abs_val(val) < MATE) return 0;
  if (val > MATE) return ((DEAD - val + 1) >> 1);
  else return (-((DEAD + val) >> 1));
}


//====================================================================
//PVUpdate() & PVDisplay()  manage the principal variation.  nih - the
//method to update the pv is from Faile (Adrien Regimbald).  as we
//ascend the search tree the move played (X) is saved in the main
//diagonal of a square array:
//       |X    |          |X    |          |X    |
// ply 1 |     |    ply 2 |  X  |    ply 3 |  X  |
//       |     |          |     |          |    X|
//as we find pv moves (P) and descend we copy the row (of length len)
//to the row below and increment the length:
//       |X    |          |X    |          |P P P|
// ply 3 |  X  |    ply 2 |  P P|    ply 1 |     |
// len=1 |    P|    len=2 |     |    len=3 |     |
//a companion 1-dimensional array keeps track of the length.
//====================================================================
void PVDisplay(int score, int mark) {
  int val, j;
  char rm[10];

  if (!xb_post) return;
  // Time is reported to the nearest centisecond:
  int et = (int)((Now() - tc_start + 5)/10);

  //format the root move
  strcpy(rm, Move2XBoard(pv_move[0][0]));
  j = Len(rm);
  if (mark) {
    if (mark > 0) rm[j] = '!';
    else rm[j] = '?';
    j++;
    rm[j]=0;
  }
  //simon bases mate scores on the value MATE.  winboard expects
  //score based on 32767 (INF).  I like to keep MATE and INF well 
  //apart to avoid unexpected consequences.  Here we adjust the 
  //score to winboard expected value.
  if (abs_val(score) > DEAD) {
    if (score > 0) score += 32767-MATE;
    else score -= 32767-MATE;
  }
  printf("%d %d %ld %ld %s", iter, score, et, nodes, rm);
  
  
  
  for (j=1; j<pv_len[0]; j++) {
    printf(" %s", Move2XBoard(pv_move[0][j]));
  }
  printf("\n");    //send \n & flush buffer
  fflush(stdout);
} //PVDisplay();

void PVUpdate(int ply, int move) {
  int j;
  pv_move[ply][ply] = move;
  pv_len[ply] = pv_len[ply+1];
  for (j=ply+1; j<pv_len[ply+1]; j++) {
    pv_move[ply][j] = pv_move[ply+1][j];
  }
}

//====================================================================
//Draw3Rep() returns TRUE if the position is a draw by 3 repetition.
//To repeat a position you have to go somewhere and come back so a
//minimum of 9 positions (g_ply >= 8) are required. following a
//capture or pawn advance (which resets g_ply to 0) we have a
//repeatable position, thus (xx=other color, yy=other pos):
//g_ply:     0  1  2  3  4  5  6  7  8
//position: r1 xx yy xx r2 xx yy xx r3 
//if first_rep is TRUE will return TRUE on the first repetition.
//====================================================================
bool Draw3Rep(int ply, int first_rep) {
  int i, j;
  if (g_ply[ply] < 4) return FALSE;
  i = ply - 4;    //first possible repeat is 4 plys back
  j = g_ply[ply] - 4;
  //step back through search tree
  while (i >= 0) {
    if (j < 0) return FALSE;
    if (key_1 == tree[i].key1) {
      if (first_rep) return TRUE;
      first_rep = TRUE;
    }
    i -= 2;
    j -= 2;
  }
  i += hix;
  //continue back through game history
  while (i >= 0) {
    if (j < 0) return FALSE;
    if (key_1 == hist[i].key1) {
      if (first_rep) return TRUE;
      first_rep = TRUE;
    }
    i -= 2;
    j -= 2;
  }
  return FALSE;
}

//===================================================================
//CanWin() insufficient material check
//1 white can win
//2 black can win
//3 both can win
//===================================================================
int CanWin() {
  int can_win = 0;
  if (w_pawns || w_queens || w_rooks || (w_bishops > 1) ||
    (w_bishops && w_knights)) can_win |= 1;
  if (b_pawns || b_queens || b_rooks || (b_bishops > 1) ||
    (b_bishops && b_knights)) can_win |= 2;
  return can_win;
}

//====================================================================
//Board() displays ascii board
//====================================================================
void Board(int black_side) {
  
  if (!analysis_mode) {
                      
  char sep[] = "  +---+---+---+---+---+---+---+---+";
  char fil[] = "    a   b   c   d   e   f   g   h";
  char c;
  int i,j, f1=0, r1=7, d1=-1;
  if (black_side) {
    r1 = 0;
    f1 = 7;
    d1 = 1;
    strcpy(fil, "    h   g   f   e   d   c   b   a");
  }
  printf("\n%s\n",sep);
  for (i=1; i<=8; i++) {
    printf ("%d |", r1+1);
    for (j=1; j<=8; j++) {
      c = (char) board[square(r1, f1)];
      if (c) {
        if (c & KTC) {
          printf("<%c>", men_lower[c & TYPE]);
        } else {
          printf("[%c]", men_upper[c]);
        }
      } else printf("   ");
      printf ("|");
      f1 -= d1;
    }
    printf("\n%s\n",sep);
    f1 += d1 << 3;
    r1 += d1;
  }
  printf("%s\n", fil);
}
}

//==================================================================
//Move2XBoard() returns winboard from-to-promotion format string
//==================================================================
const char *Move2XBoard(int move) {
  static char buf[6];
  char *pc = buf;
  *pc++ = (char) ('a' + col(mv_b1(move)));
  *pc++ = (char) ('1' + row(mv_b1(move)));
  *pc++ = (char) ('a' + col(mv_b2(move)));
  *pc++ = (char) ('1' + row(mv_b2(move)));
  switch (mv_pro(move)) {
  case KNIGHT:
    *pc++ = 'n';
    break;
  case BISHOP:
    *pc++ = 'b';
    break;
  case ROOK:
    *pc++ = 'r';
    break;
  case QUEEN:
    *pc++ = 'q';
    break;
  }
  *pc = 0;
  return (buf);
}

//====================================================================
//strip leading whitespace from buffer
//====================================================================
void Strip(char *buf) {
  int i, j=0, n = strlen(buf);
  while (isspace((int) buf[j])) j++;
  if (j) for(i=0; i <= n-j; i++) buf[i] = buf[i+j];
}

//====================================================================
//SortBubble() sorts num moves (num = 0 sorts all moves)
//====================================================================
void SortBubble(s_move *pm1, s_move *pm2, int num) {
  int swap;
  s_move  temp, *pm;
  pm2--;    //last move
  while (pm1 < pm2) {
    pm = pm2;
    swap = FALSE;
    while (pm > pm1) {
      if (pm->val > (pm-1)->val) {
        temp = *pm;
        *pm = *(pm-1);
        *(pm-1) = temp;
        swap = TRUE;
      }
      pm--;
    }
    num--;
    if (!swap || !num) return;
    pm1++;
  }
}

//====================================================================
//SortReOrder() moves pm2 to pm1 with all moves in between moved down
//====================================================================
void SortReOrder(s_move *pm1, s_move *pm2) {
  s_move temp;
  if (pm2 <= pm1) return;
  temp = *pm2;
  while (pm2 > pm1) {
    *pm2 = *(pm2 -1);
    pm2--;
  }
  *pm1 = temp;
}

