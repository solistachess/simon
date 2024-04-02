//hash.cpp by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.
#include "chess.h"

/*********************************************************************
File contains functions to manage the transposition hash table.  The
hash table is simply a large array of hash_entry structs.  Each entry
has two values: the position key and the data.  The purpose of the 
hash table is to save information that we gather during the search. 
If the position arises again due to a transposition we can use the
data to avoid having to research the position.

The table is two part and works as follows:  Each key has two "slots". 
Slot 1 has the "better" data.  New data is always wrtten to the table. 
New data goes into slot 1 if it is empty or occupied by the current 
position.  If slot 1 is occupied by a different position we determine 
(based on depth) which data looks better.  If old data looks better 
it stays in slot 1 and new data goes in slot 2.  If new data looks 
better it goes in slot 1 and old data is moved to slot 2.

To probe the table we first try slot 1.  If the position does not 
match we then try slot2.

Hash data is a bit field as follows:
  bits                  shift  mask      note
  --------------------  -----  ----      ------------------
   2 - type             0      3         bounds - HF_xxx
   1 - threat           2      1         null move threat
  17 - unsigned score   3      1ffff     score + 65536
  12 - depth            20     fff
  28 - move             32     fffffff   26 used by search
   1 - old              60               
   3 - unused           63

Note:  simon does not use the threat field and the depth field is far
larger than needed.  I took the hash routines from my program Bruja 
and did not want to screw with them since hash bugs are easy to 
introduce and hard as hell to find and fix.  The threat field will be 
useful if you implement a null move and the larger depth field allows 
fractional ply extensions.
**********************************************************************/

//macros to insert and extract data
#define hget_type(d)   ((int)  ((d)&3))
#define hget_threat(d) ((int) (((d)>>2) & 1))
#define hget_val(d)   (((int)  ((d)>>3) & 0x1ffff)-65536)
#define hget_depth(d)  ((int) (((d)>>20) & 0xfff))
#define hget_move(d)   ((int) (((d)>>32) & 0xfffffff))

#define hset_type(a)   (a)
#define hset_threat(a) (((U64)  (a))<<2)
#define hset_val(a)    (((U64) ((a)+65536))<<3)
#define hset_depth(a)  (((U64)  (a))<<20)
#define hset_move(a)   (((U64)  (a))<<32)

typedef struct {
  U64 key1;
  U64 data;
} hash_entry;
static hash_entry no_table[2];              //avoid invalid pointer
static hash_entry * hash_table = no_table;  //hash table
static unsigned int hash_nel = 0;           //number of elements
static unsigned int hash_mask = 0;          //mask
static const U64 old_entry = 0x1000000000000000;  //old entry flag

//====================================================================
//InitHash() is called at program startup to allocate memory for the
//hash table.  It also initializes our random number rnd_xxx 
//variables which are used to build hash keys.  InitHash() is passed 
//the desired table size in mb and returns the mb actually 
//allocated.  Unfortunately with Windows (far as I know) the 
//allocation never fails - if physical ram is not available it uses
//virtual memory which will slow the program to a crawl.  If you 
//know a way to have memory allocation fail if memory is not 
//available please let me know.
//====================================================================
int InitHash(int hash_mb) {
  extern U64 *rnd_num;
  const int min_mb = 2;   //minimum hash table size
  int i,j;
  //cast rnd_num[] to rnd_psq[16][64] and copy random numbers
  //from (unused) row 0 to rnd_epc.  rnd_epc[0] stays 0 so it doesn't
  //have to be hashed out when reset.
  rnd_psq = (U64 (*)[64]) rnd_num;
  for (i=1; i<48; i++) rnd_epc[i] = rnd_psq[0][i];
  rnd_btm = rnd_psq[0][48];
  //allocate largest valid mb value (power of 2) not greater than
  //requested hash_mb
  j = sizeof(hash_entry);
  // Loop until we hit the break by passing the test
  for ( ; ; ) {
    if (hash_mb < min_mb) {
      hash_nel = 0;
      hash_mask = 0;
      hash_table = no_table;
      break;
    }
    hash_nel = 65536;
    while (hash_nel <= (unsigned) ((hash_mb * 524288) / j))
      hash_nel = hash_nel << 1;
    hash_mask = (hash_nel - 1)^1;
    hash_table = (hash_entry*) malloc(hash_nel*j);
    if (hash_table != NULL) break;
    //allocation fails - reduce mb and try again
    hash_mb--;
  }
  ClearHash();
  hash_mb = (j * hash_nel)/1048576;
  return hash_mb;
}

//====================================================================
//FreeHash() called at program termination to free hash memory
//====================================================================
void FreeHash() {
  if (hash_table != no_table) free(hash_table);
}

//====================================================================
//ClearHash() clears the hash table, history heuristic & killers.
//====================================================================
void ClearHash(void) {
  unsigned i;
  size_t bytes = hash_nel * sizeof(hash_entry);
  if (bytes) memset(hash_table, 0, bytes);

  //clear the history heuristic
  for (i = 0; i < 4096; i++) {
    hh_white[i] = hh_black[i] = 0;
  }
  hh_max = 0;
  //clear  killers
  for (i = 0; i < MAX_PLY; i++) {
    tree[i].killer1 = 0;
    tree[i].killer2 = 0;
  }
}

//====================================================================
//AgeHash() "ages" the hash table.by setting all entrys to old.
//Also ages history heuristic and clears killers from the search 
//tree.  The hash table is cleared on occasion unless a 50 move draw
//is approaching, in which case it is cleared every move.
//====================================================================
void AgeHash(void) {
  unsigned i;
  for (i=0; i < hash_nel; i++) hash_table[i].data |= old_entry;
  //age history
  for (i = 0; i < 4096; i++) {
    hh_white[i] = hh_white[i] >> 8;
    hh_black[i] = hh_black[i] >> 8;
  }
  hh_max = hh_max >> 8;
  //clear killers
  for (i = 0; i < MAX_PLY; i++) {
    tree[i].killer1 = 0;
    tree[i].killer2 = 0;
  }
}

//====================================================================
//HashStore() and HashProbe() save and retrieve information from
//the hash table
//====================================================================
void HashStore(int ply, int depth, int type, int threat, int val, int move) {
  //index slot
  hash_entry *ph;
  ph = hash_table + (key_1 & hash_mask);
  //adjust for mate
  if (val > DEAD) val += ply;
  else if (val < -DEAD) val -= ply;

  if (ph->key1 != key_1) {
    //slot 1 is empty or occupied by a different position
    if ((ph->data & old_entry)||(depth >= hget_depth(ph->data))) {
      //new data looks better.  get move from slot 2 if we have none
      //and then move existing occupant (if any) to slot 2
      if (!move && (ph + 1)->key1 == key_1) move = hget_move((ph+1)->data);
      (ph+1)->key1 = ph->key1;
      (ph+1)->data = ph->data;
    } else {
      //existing data in slot 1 looks better.  leave existing occupant
      //alone and put new data in slot 2
      ph++;
    }
  } else {
    //this is our slot.  save existing move if we have no new move
    if (!move) move = hget_move(ph->data);
  }
  if (depth < 0) depth = 0;   //don't try to store negative depth!
  ph->key1 = key_1;
  ph->data = hset_type(type) | hset_threat(threat) | hset_val(val) |
    hset_depth(depth) | hset_move(move);
}

int HashProbe(int ply, int &depth, int &type, int &threat, int &val, int &move) {
  hash_entry *ph;
  ph = hash_table + (key_1 & hash_mask);
  if (ph->key1 == key_1) {
    move = hget_move(ph->data);
    type = hget_type(ph->data);
    threat = hget_threat(ph->data);
    val = hget_val(ph->data);
    if (val > DEAD) val -= ply;
    else if (val < -DEAD) val += ply;
    depth = hget_depth(ph->data);
    ph->data &= ~old_entry;
    return TRUE;
  }
  ph++;
  if (ph->key1 == key_1) {
    move = hget_move(ph->data);
    type = hget_type(ph->data);
    threat = hget_threat(ph->data);
    val = hget_val(ph->data);
    if (val > DEAD) val -= ply;
    else if (val < -DEAD) val += ply;
    depth = hget_depth(ph->data);
    ph->data &= ~old_entry;
    return TRUE;
  }
  return FALSE;
}

