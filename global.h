//global.h by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.

/*********************************************************************
Global variables and function prototypes
*********************************************************************/

static bool analysis_mode = false;
static char shortmate[265] = "--> Shortest mate found!... ";

//name & version
extern const char  *name;

//=====================================================================
//position & search variables.
//=====================================================================
//basic position data
extern int color, board[], castle[], ep_sq[], g_ply[];
//bitboard position data
extern U64 bbd[16], bb_rl90, bb_rl45, bb_rr45, bb_move;
//hash keys & pin data
extern U64 key_1, key_2, pin_key1, pin_save, pin_mask[];
//piece counts, material & king locations
extern int num_men[], wk_sq, bk_sq;
//history heuristic
extern unsigned hh_white[], hh_black[], hh_max;
//move list & search tree
extern s_move   move_list[];
extern s_tree   tree[];
//pv
extern int pv_len[], pv_move[][MAX_PLY+2];

//=====================================================================
//global variables.
//=====================================================================
extern const int    p_val, n_val, b_val, r_val, q_val;
extern const int    piece_value[];

extern int          kolor, move_num, hix, game_over;

extern int          root_score;
extern int          root_moves;
extern s_move       root_list[];

//values supplied by winboard
extern int          xb_level_moves, xb_level_min, xb_level_inc;
extern int          xb_time, xb_otim, xb_st, xb_sd;
extern bool         xb_post, xb_easy, xb_mode;

//time control
extern int          tc_moves, tc_target, tc_start;

//hash keys
extern U64          (*rnd_psq)[64], rnd_epc[], rnd_btm;


extern char         err_msg[];
extern s_hist       hist[];
extern char         men_upper[], men_lower[];
//search
extern unsigned     nodes;
extern int          abort_search, iter;
extern int          draw_score;

//bitboard
extern const U64    all_64, sq_set[], sq_clr[];
extern const U64    sq_set_rl90[], sq_set_rr45[], sq_set_rl45[];

extern const U64    rank_1, rank_2, rank_3, rank_4;
extern const U64    rank_5, rank_6, rank_7, rank_8;
extern const U64    file_a, file_b, file_c, file_d;
extern const U64    file_e, file_f, file_g, file_h;
extern const U64    mask_row[8], mask_col[8];
extern const U64   *ap_bpawn, *ap_wpawn;
extern const U64    ap_knight[], ap_queen[], ap_king[];
extern const char   shr_bish_rl45[], shr_bish_rr45[];

//attack boards
extern int        (*directions)[64];
extern U64        (*obstructed)[64];
extern U64        (*ap_bish_rl45)[128], (*ap_bish_rr45)[128];
extern U64        (*ap_rook_rl90)[128], (*ap_rook_rr00)[128];

//====================================================================
//fun prototypes
//====================================================================
void          Board(int black_side = 0);
void          AddPiece(int c1, int b1);
void          AgeHash(void);
int           Attacked(int b1, int ka);
U64           Attacks(int b2);
int           CanWin();
void          ClearHash(void);
bool          Draw3Rep(int ply, int first_rep);
int           Eval();
void          FreeHash();
s_move      * GenCap(s_move *pm, int ply);
s_move      * GenEvade(s_move *pm, int ply);
s_move      * GenMov(s_move *pm, int ply);
void          GenRoot(void);
void          HashStore(int ply, int depth, int type, int threat, int val, int move);
int           HashProbe(int ply, int &depth, int &type, int &threat, int &val, int &move);
void          InitAttack(void);
int           InitHash(int hash_mb);
int           Iterate(void);
int           Len(const char *pc);
int           MakeMove(int move);
void          Move(int move, int ply);
void          MoveNull(int ply);
const char  * Move2XBoard(int move);
void          Play(void);
void          Print(const char *fmt, ...);
void          PVDisplay(int score, int mark);
void          PVUpdate(int ply, int move);
bool          SetBoard(char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
int           Sex(int move);
void          ShutDown(int status);
void          SortBubble(s_move *pm1, s_move *pm2, int num=0);
void          SortReOrder(s_move *pm1, s_move *pm2);
void          Strip(char *buf);
int           UnMakeMove();
void          UnMove(int move, int ply);
void          UnMoveNull(int ply);
void          UpdateHistory(int move, int ply, int depth);
int           Val(const char *pc);
