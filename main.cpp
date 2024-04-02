//main.cpp by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.
#include "chess.h"

/*********************************************************************
File contains program start function main() and initialization.
Also included are some assorted test and debugging functions.
*********************************************************************/


//====================================================================
//BitBoard() displays bits in 64 bit number as a board.
//====================================================================
void BitBoard(U64 num) {
  int r, c;
  char buf[9] = {0};
  for (r = 7; r >= 0; r--) {
    for (c = 0; c <= 7; c++) {
      if (sq_set[square(r,c)] & num)
        buf[c] = '1';
      else
        buf[c] = '0';
    }
    printf("\n%d %s", r+1, buf);
  }
  printf("\n  abcdefgh\n");
}

//====================================================================
//Perft() is used to test the move generator.  It plays all moves and 
//and replys to depth and counts them.  From the starting position 
//a perft to depth 1 is 20 and to depth 2 is 400.
//Call w/depth >= 1 and ply = 0.
//====================================================================
unsigned Perft(int depth, int ply) {
  assert((depth > 0) && (ply < MAX_PLY));
  s_move temp[256];
  s_move *pm1 = temp, *pm2;
  int move;
  unsigned num=0;
  if (incheck) 
    pm2 = GenEvade(pm1, ply);
  else 
    pm2 = GenMov(GenCap(pm1, ply), ply);
  if (depth == 1) return pm2-pm1;  //done
  for (; pm1 < pm2; pm1++) {
    move = pm1->move;
    Move(move, ply);
    num += Perft(depth-1, ply+1);
    UnMove(move, ply);
  }
  return (num);
}

//====================================================================
//TestGen() uses a well known position with all types of special 
//moves as a torture test for the move generator.
//====================================================================
void TestGen() {
  unsigned num, ans[6] = {0,48,2039,97862,4085603,193690690};
  SetBoard("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 8");
  Board();
  for (int i = 1; i <= 5; i++) {
    num = Perft(i, 0);
    printf("Perft(%d)=%d   ans=%d   ", i, num, ans[i]);
    if (num != ans[i]) {
      Print("ERROR");
      return;
    }
    Print("OK");
  }
}

//====================================================================
//64 bit random number generator for hash keys by Bruce Moreland.
//====================================================================
U64 Rand64(void) {
    return rand()^((U64)rand()<<15)^((U64)rand()<<30)^
                  ((U64)rand()<<45)^((U64)rand()<<60);
}

//=====================================================================
//InitMem() allocates the following global arrays from the free store.
//These, plus the transposition hash table are the only memory that
//simon allocates dynamically.
//=====================================================================
int (*directions)[64];            //directions from square to square
U64 (*obstructed)[64];            //obstructed square mask
U64 (*ap_bish_rl45)[128];         //rotated bitboard attack boards
U64 (*ap_bish_rr45)[128];
U64 (*ap_rook_rl90)[128];
U64 (*ap_rook_rr00)[128];
U64 *rnd_num;

void InitMem(void) {
  directions = new int[64][64];
  obstructed = new U64[64][64];
  ap_bish_rl45 = new U64[64][128];
  ap_bish_rr45 = new U64[64][128];
  ap_rook_rl90 = new U64[64][128];
  ap_rook_rr00 = new U64[64][128];
if (bruja_book) {
  extern U64 random_numbers[];
  rnd_num = random_numbers;
}else{
  rnd_num = new U64[1024];
  for (int i = 0; i < 1024; i++) rnd_num[i] = Rand64();
}
} //InitMem()

//=====================================================================
//ShutDown() frees memory and terminates program.
//=====================================================================
void ShutDown(int status) {
  delete[] directions;
  delete[] obstructed;
  delete[] ap_bish_rl45;
  delete[] ap_bish_rr45;
  delete[] ap_rook_rl90;
  delete[] ap_rook_rr00;
if (!bruja_book) {
  delete[] rnd_num;
}
  FreeHash();
  exit(status);
} //ShutDown()

//=====================================================================
//main() performs startup initialization then branches to Play()
//=====================================================================

int CDECL
 main(int argc, char *argv[])  {  
  int i;
  int mb = 16;                      //default hash table size  
  InitMem();                        //allocate memory for attack boards
  if (argc > 1) mb = Val(argv[1]);  //get hash from cmd line
  if (mb < 4) mb = 4;               //min if bad cmd line arg
  mb = InitHash(mb);                //allocate hash memory
  
  for (i = 1; i < argc; i++) {
  if (strcmp(argv[i], "-book") == 0) {
            bruja_book    = true;
            }else{
            bruja_book    = false;
            }
  }
  Print("\n%s by Dan Honeycutt [01/27/05]",name); //identify ourselves
  Print("v1.3 additions by Jim Ablett   [07/07/10]"); 
  if (bruja_book) {
  Print("internal book is on");
}else{
  Print("internal book is off");
}
  Print("%d mb hash\n",mb);
  Print("feature setboard=1 time=1");
  Print("feature variants=\"normal,nocastle\"");
  Print("feature sigint=0 sigterm=0 colors=0");
  Print("feature reuse=0 analyze=1 done=1\n");
  InitAttack();                     //initialize attack boards
  SetBoard();                       //set board to start position
  Play();                           //play the game
  ShutDown(0);                      //end
  return 0;
}


//can be used for debugging - convert 0-63 into square names
/*
const char *sq[] = {"a1","b1","c1","d1","e1","f1","g1","h1",
                    "a2","b2","c2","d2","e2","f2","g2","h2",
                    "a3","b3","c3","d3","e3","f3","g3","h3",
                    "a4","b4","c4","d4","e4","f4","g4","h4",
                    "a5","b5","c5","d5","e5","f5","g5","h5",
                    "a6","b6","c6","d6","e6","f6","g6","h6",
                    "a7","b7","c7","d7","e7","f7","g7","h7",
                    "a8","b8","c8","d8","e8","f8","g8","h8"};

*/













////////// play.cpp ////////////////////


/*********************************************************************
File includes the main play loop, Play() and assorted support
functions.  Play() gets input and calls GetCmd() to see if the entry
is a recognized command.  If so Play() handles the command.  If not 
Play() calls GetMove().  If the entry is not a move we simply ignore
it (it is most likely an unimplemented command) and go back for more
input.  If it is a move we play it.  After playing the move Play() 
branches based on force mode (force mode is winboard's way of saying
"don't start thinking yet").  If in force mode we go back for more 
input, if not we call Iterate() to get the computer reply.

Below is the list of commands which simon implements (or understands
but ignores).  Comments in the code give a brief description of their 
purpose.  For complete information see the file engine-intf.html 
available from Tim Mann's winboard site.
*********************************************************************/

#define CMD_QUIT      1
#define CMD_BOARD     2
#define CMD_NEW       3
#define CMD_GO        4
#define CMD_POST      5
#define CMD_NOPOST    6
#define CMD_QM        7
#define CMD_EASY      8
#define CMD_HARD      9
#define CMD_XBOARD    10
#define CMD_PROTOVER  11
#define CMD_SD        12
#define CMD_LEVEL     13
#define CMD_TIME      14
#define CMD_OTIM      15
#define CMD_RANDOM    16
#define CMD_UNMOVE    17
#define CMD_FORCE     18
#define CMD_ACCEPTED  19
#define CMD_RESULT    20
#define CMD_WHITE     21
#define CMD_BLACK     22
#define CMD_COMPUTER  23
#define CMD_SETBOARD  24
#define CMD_ANALYZE   25
#define CMD_EXIT      26
#define CMD_PING      27
#define CMD_ST        28
#define CMD_HELP      29

static char const *cmds[] = {   //this list must match CMD_XXX
  "dummy",
  "quit",
  "board",
  "new",
  "go",
  "post",
  "nopost",
  "easy",
  "hard",
  "xboard",
  "protover",
  "exit",
  "sd",
  "level",
  "time",
  "otim",
  "random",
  "unmove",
  "force",
  "accepted",
  "result",
  "white",
  "black",
  "computer",
  "setboard",
  "analyze",
  "exit",
  "ping",
  "st",
  "help",
  "variant nocastle",
  ".",
  "?",
  NULL};


//==================================================================
//GetCmd() returns CMD_XXX or 0 if unrecognized.  if command is 
//valid the entry will point to any args following.
//==================================================================
int GetCmd(char *entry) {
  const int cmd_len = 12;       //max len of command (not incl args)
  int i, n, cmd = 0;
  char buf[cmd_len+1];
  Strip(entry);                     //strip leading whitespace
  for (n = 0; n < cmd_len; n++) {   //copy to first space, tab, nl
    buf[n] = (char) tolower(entry[n]);     //fold to lowercase
    if ((buf[n] == 0) || isspace((int) buf[n])) {
      buf[n] = 0;                   //zero terminate
      break;
    }
  }
  if ((n == 0) || (n == cmd_len)) return 0;
  for (i=1; cmds[i] != NULL; i++) {
    if (strcmp(buf, cmds[i])==0) {
      cmd = i;                      //found a match
      break;
    }
  }
  if (!cmd) return 0;               //no match found
  for (i=0; i<=Len(entry)-n; i++) entry[i]=entry[i+n];
  Strip(entry);
  return (cmd);
}

//==================================================================
//GetMove() returns returns root move corresponding to entry.
//entry must be in winboard format e2e4 or e7e8q.
//==================================================================
int GetMove(const char *entry) {
  const int mov_len = 6;            //max len of move
  int i, move, n=0;
  short mov;
  char buf[mov_len+1];
  for (n = 0; n < mov_len; n++) {   //copy to first space, tab, nl
    buf[n] = (char) tolower(entry[n]);     //fold to lowercase
    if ((buf[n] == 0) || isspace((int) buf[n])) {
      buf[n] = 0;                   //zero terminate
      break;
    }
  }
  if ((n < 4) || (n > 5)) return 0;
  mov = buf[0] - 'a' + 8*(buf[1]-'1');           //from
  mov +=  64*(buf[2] - 'a' + 8*(buf[3]-'1'));    //to
  if (n == 5) {
    switch (buf[4]) {
    case 'q':
      mov |= QUEEN * 4096;
      break;
    case 'r':
      mov |= ROOK * 4096;
      break;
    case 'b':
      mov |= BISHOP * 4096;
      break;
    case 'n':
      mov |= KNIGHT * 4096;
      break;
    default:
      return 0;
    }
  }
  for (i = 0; i < root_moves; i++) {
    move = root_list[i].move;
    if (mov == (short) move) return move;
  }
  return 0;
}


//====================================================================
//BookMove() translates the board position to CBookPos and
//calls CBook::GetBookMove() for move.
//====================================================================
#include "book.h"
static CBook book;    //we need one CBook at global scope ...
int BookMove() {
  CBookPos bp;        //and one CBookPos to hold current position
  char c=0;
  int i, b;
  bp.bk_color = (char) color;          //color moving
  bp.bk_ep_target = (char) ep_sq[0];   //enpassant target
  for (i = 0; i < 64; i++) {
    b = board[i];               //b = simon piece value
    switch (b & TYPE) {
    case ESQ:
      c = 0;                    //c = CBook piece value
      break;
    case PAWN:
      c = 1;
      break;
    case KNIGHT:
      c = 2;
      break;
    case BISHOP:
      c = 3;
      break;
    case ROOK:  //rook is the only piece that requires work
      c = 4;    //assume a normal rook.  see if it can castle
      if (b & BLACK) {
        if (((i==A8) && (castle[0] & 8)) ||
            ((i==H8) && (castle[0] & 4))) c = 7;
      } else {
        if (((i==A1) && (castle[0] & 2)) ||
            ((i==H1) && (castle[0] & 1))) c = 7;
      }
      break;
    case QUEEN:
      c = 5;
      break;
    case KING:
      c = 6;
      break;
    }
    if (b & BLACK) c += 8;
    bp.bk_board[i] = c;           //place piece on board
  } //for board[]

  //Translation complete.  GetMove() will parse the book move string
  //and look up the move in the root move list to verify validity.
  return GetMove(book.GetBookMove(&bp));
}


//====================================================================
//SendFeatures() is called if the winboard protocall version is 2 or
//higher (which it best be since simon does not support protover 1).
//Here we tell winboard what features we support.
//====================================================================
void SendFeatures() {
  Print("feature done=0");
  //if the engine has some initialization function which takes a long
  //time (which simon doesn't) it can be done here.  Once we send
  //feature done=0 winboard is going to wait for feature done=1
  //before it does anything.
  Print("feature ping=0 setboard=1 time=1 variants=normal,nocastle");
  Print("feature sigint=0 sigterm=0 colors=0 analyze=1");
  Print("feature myname=\"%s\"", name);
  Print("feature done=1");
}

//====================================================================
//SendResult() sends game result when a move ends the game or we
//decide to throw in the towel
//====================================================================
void SendResult() {
  switch (game_over) {
  case FIN_BLACK_MATED:
    Print("1-0 {Checkmate}");
  case FIN_WHITE_MATED:
    Print("0-1 {Checkmate}");
    break;
  case FIN_BLACK_RESIGNS:
    Print("1-0 {Black resigns}");
    break;
  case FIN_WHITE_RESIGNS:
    Print("0-1 {White resigns}");
    break;
  case FIN_STALEMATE:
    Print("1/2-1/2 {Stalemate}");
    break;
  case FIN_3_REP:
    Print("1/2-1/2 {3 repetitions}");
    break;
  case FIN_50_MOVE:
    Print("1/2-1/2 {50 move rule}");
    break;
  case FIN_LACK_FORCE:
    Print("1/2-1/2 {Insufficient force}");
    break;
  }
}

//====================================================================
//Resign() looks to see if we are getting crushed and should resign.
//Nothing says we have to resign but tournament directors appreciate
//this as it speeds their tournaments if the engine does not play on
//in hopeless situations.
//====================================================================
int Resign() {
  if (game_over) return 0;    //no need resign if game ended
  if (root_score < -600) {  
    //things look bad based on the score but we don't give up
    //unless we're down more than a rook in raw material
    if (kolor == WHITE) {
      if (black_mtl-white_mtl>r_val) return FIN_WHITE_RESIGNS;
    } else {
      if (white_mtl-black_mtl>r_val) return FIN_BLACK_RESIGNS;
    }
  }
  return 0;
}

//====================================================================
//Play() main play loop.
//====================================================================
void Play(void) {
  char ibuf[512];                   //input buffer
  int cmd, move, force = FALSE;
  int tm;
  int rm;
get_input:
  if (!xb_mode) {         //console mode - prompt for input
    if (game_over) printf("cmd: ");
    else {
      if (xb_mode && !analysis_mode) {
      Board(kolor ^ KTC);
      if (color) printf("%d . . . ", move_num);
      else printf("%d ", move_num);
      }
    }
  }
  if (fgets(ibuf, sizeof(ibuf), stdin) != ibuf) {
    if (feof(stdin)) ShutDown(1);   //input pipe lost
  }
  //check for command
  cmd = GetCmd(ibuf);
  if (cmd) {
    switch (cmd) {
    default:            //cmds which we recognize but ignore
      goto get_input;
    case CMD_BOARD:     //for debugging - not a winboard command
      Board();
      goto get_input;
    case CMD_EASY:      //tells us not to ponder.  we just record
      xb_easy = TRUE;   //this since simon doesn't ponder
      goto get_input;
    case CMD_ANALYZE:
      force = TRUE;
      analysis_mode = TRUE;
      bruja_book = FALSE;
      break;
    case CMD_EXIT:
      analysis_mode = FALSE;
      force = FALSE;
      break;    
    case CMD_QUIT:      //the end
      return;
    case CMD_FORCE:     //tells us not to move till we get a go
      force = TRUE;
      goto get_input;
    case CMD_GO:        //tells us to move from the current position
      force = FALSE;
      break;
    case CMD_HARD:      //opposite of easy
      xb_easy = FALSE;
      goto get_input;
    case CMD_LEVEL:     //set tournament or incremental time control
      sscanf(ibuf, "%d %d %d", &xb_level_moves, &xb_level_min, &xb_level_inc);
      tc_moves = xb_level_moves;
      if ((xb_level_moves < 0)||(xb_level_inc < 0)||(xb_level_min < 1))
        xb_st = 2;      //level args bad, think for 2 sec
      else
        xb_st = 0;      //level cmd looks OK, use it for time control
      goto get_input;
    case CMD_XBOARD:    //normally the first command received
      xb_mode = TRUE;
      force = TRUE;     //set force and await further instructions
      goto get_input;
    case CMD_NEW:       //start a new game
      SetBoard();
      force = FALSE;
      goto get_input;
    case CMD_NOPOST:    //opposite of post
      xb_post = FALSE;
      goto get_input;
    case CMD_OTIM:      //opponents time remaining.  we just note it
      xb_otim = Val(ibuf);
      goto get_input;
    case CMD_PING:      //winboard sends ping to synchronize
      Print("pong %s", ibuf); //we pong to say we are alive and well
      goto get_input;
    case CMD_POST:      //tells us to post results of our wearch
      xb_post = TRUE;
      goto get_input;
    case CMD_PROTOVER:  //normally received right after xboard
      if (Val(ibuf) >= 2) SendFeatures();
      goto get_input;
    case CMD_RESULT:    //game ended (hopefully opponent resigned)
      game_over = -1;
      goto get_input;
    case CMD_SETBOARD:  //setup board to fen position
      game_over = -1;
      if (!SetBoard(ibuf)) Print("Error (%s): setboard", err_msg);
      goto get_input;
    case CMD_SD:        //set maximum search depth
      xb_sd = Val(ibuf);
      goto get_input;
    case CMD_ST:        //set fixed time sec/move
      xb_st = Val(ibuf);
      goto get_input;
    case CMD_TIME:      //our time remaining
      xb_time = Val(ibuf);
      goto get_input;
    case CMD_UNMOVE:    //for debugging - not a winboard command
      if (UnMakeMove()) game_over = 0;
      else Print("Not enough history");
      goto get_input;
    case CMD_HELP:      //our time remaining
	  {
	  size_t hind = 0;
      while (cmds[hind])
		  puts(cmds[hind++]);
	  }
      goto get_input;
    }
  } else {
    //unrecognized command - try a move
    if (game_over) goto get_input;
    move = GetMove(ibuf);
    if (!move) goto get_input;
    game_over = MakeMove(move);     //play the move
    if (game_over && !analysis_mode) {
    SendResult();
}else
   if (game_over) {
  // Print(shortmate);
    Print("0, 0, 0, 0 %s", shortmate);
}
      
    if (force) goto get_input;
  }
  //computers turn to move
  if (game_over) goto get_input;
  kolor = color;

if (bruja_book) {
  move = move = BookMove();
}else{
  move = 0;
}
  if (move) Print("0 0 0 0 (Book)");
  else move = Iterate();
  
  
  if (!analysis_mode) {
  Print("move %s", Move2XBoard(move));
  } 
    
  game_over = MakeMove(move);
  
  if (analysis_mode && game_over) Print("0, 0, 0, 0 %s", shortmate); //Print(shortmate);
  
  if (!analysis_mode) {
  if (!game_over) game_over = Resign();
  if (game_over) SendResult();
}
 
  

      
  if (xb_level_moves > 0) {
    tc_moves--;
    if (tc_moves <= 0) tc_moves = xb_level_moves;
  }
  goto get_input;
}


