//search.cpp by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.
#include "chess.h"

//====================================================================
//SetSearchTime() sets our time allocation, tc_target
//====================================================================
static void SetSearchTime() {
  int total, avg, max;
  tc_start = Now();
  abort_search = FALSE;
  if (xb_st > 0) {                      //fixed sec/move
    tc_target = xb_st * 1000 - 100;
    return;
  }
  total = xb_level_min * 60000;         //total time, ms
  if (!xb_level_moves) {
    //incremental time control
    tc_target = total / 100;            //plan for 100 moves
    tc_target += 1000 * xb_level_inc;   //add increment if any
  } else {
    //tournament time control
    tc_target = avg = total / xb_level_moves;
    if ((xb_time > 0) && (tc_moves > 0)) {
      //have time and moves remaining data
      tc_target = xb_time * 10 / tc_moves;
      max = xb_time / 2;
      if (tc_target > avg) {
        //we have some time surplus
        tc_target = 3 * tc_target / 2;
      }
      if (tc_target > max) tc_target = max;
    }
  }
}

//====================================================================
//CheckClock() is called periodically by Search() to see if our
//time is spent, in which case we set abort_search
//====================================================================
static void CheckSearchTime() {
  int et = Now() - tc_start;     //time spent
  abort_search = et > tc_target;
}

//====================================================================
//IsDraw() returns TRUE if current position is a draw.
//====================================================================
static bool IsDraw(int ply) {
  s_move temp[256];
  
  if (analysis_mode)  return (!CanWin());
  
  if (g_ply[ply] >= 4) {
    //during search (ply > 1) we call a single repetition a draw
    if (Draw3Rep(ply, (ply>1))) return TRUE;
    if (g_ply[ply] >= 100) {
      //draw unless the 50th move produces mate
      if (incheck) {
        if (temp == GenEvade(temp, ply)) return FALSE;
      }
      return TRUE;
    }
  } //game ply >= 8
  return (!CanWin());
}

//====================================================================
//QSearch() seeks to obtain a quiet position so our evaluation won't
//be distorted by pending captures.  To do that it searches only
//capture moves.
//====================================================================
int QSearch(int alpha, int beta, int ply) {
  int stand_pat, val, best_val, move;
  s_move *pm, *pm1, *pm2;

  //housekeeping
  nodes++;
  pv_len[ply] = ply;
  if (ply >= MAX_PLY) return beta;
  if (!(nodes & 1023)) {  //check in every 1000 nodes
    CheckSearchTime();
    if (abort_search) return 0;
  }

  //stand_pat is the score we return if we find no worthwhile captures
  //if stand_pat is above beta we return right away - we can't kill
  //them any deader.
  stand_pat = Eval();
  if (stand_pat > alpha) {
    if (stand_pat >= beta) return stand_pat;
    alpha = stand_pat;
  }

  //generate moves
  pm1 = tree[ply-1].pm2;
  
  if (!analysis_mode) {
  if (incheck) {
    //in check
    pm2 = GenEvade(pm1, ply);
    if (pm2 == pm1) return -MATE + ply;   //checkmate
  } else {
    //not in check
    pm2 = GenCap(pm1, ply);
    if (pm2 == pm1) return stand_pat;     //no captures available
  }
}



  //look for best capture and prune those that lose material
  best_val = 0;
  pm = pm1;
  while (pm < pm2) {
    move = pm->move;
    val = Sex(move);
    if (val < 0) {
      pm2--;
      *pm = *pm2;
      continue;
    }
    if (val > best_val) {
      best_val = val;
      SortReOrder(pm1, pm);
    }
    pm++;
  }

  //set end of our moves for next ply and search them
  tree[ply].pm2 = pm2;
  for (pm = pm1; pm < pm2; pm++) {
    move = pm->move;
    Move(move, ply);
    val = -QSearch(-beta, -alpha, ply+1);
    UnMove(move, ply);
    if (abort_search) return 0;
    if (val > alpha) {
      if (val >= beta) {
        return val;
      }
      PVUpdate(ply, move);
      alpha = val;
    }
  } //for moves
  
  return alpha;
}

//====================================================================
//Search() performs the recursive alpha-beta search.  This is a very
//basic search with no null move or extensions but it does utilize 
//hash table, history heuristic and killers.  It also maintains the 
//principal variation and handles draws and mate.
//====================================================================
int Search(int alpha, int beta, int depth, int ply) {
  int val, move, best_move, h_type, h_depth;
  int h_threat = 0;   //this will have use if you implement null move
  s_move *pm, *pm1, *pm2;
  unsigned hh_fact, *hh;

  //housekeeping
  nodes++;
  pv_len[ply] = ply;
  if (ply >= MAX_PLY) return beta;
  if (!(nodes & 1023)) {  //check in every 1000 nodes
    CheckSearchTime();
    if (abort_search) return 0;
  }
  tree[ply].key1 = key_1; //update search tree for draw detection
  if (IsDraw(ply) && !analysis_mode) return draw_score;
  
  //see if we can get a quick cutoff from the hash table
  best_move = 0;
  if (HashProbe(ply, h_depth, h_type, h_threat, val, best_move)) {
    if ((h_depth >= depth) || (val > DEAD && h_type != HF_UPPER)) {
      switch (h_type) {
      case HF_LOWER:
        if (val >= beta) return val;
        break;
      case HF_UPPER:
        if (val <= alpha) return val;
        break;
      case HF_EXACT:
        if (val > alpha && val < beta && best_move) {
          pv_move[ply][ply] = best_move;
          pv_len[ply] = ply+1;
        }
        return val;
      }
    } //if enough depth
  } //if hash probe

  //No hash cut but we may have a move to try.  We now generate moves
  pm1 = tree[ply-1].pm2;
  if (!analysis_mode) {
  if (incheck) {
    //in check
    pm2 = GenEvade(pm1, ply);
    if (pm2 == pm1) return -MATE + ply;   //checkmate
  } else {
    //not in check
    pm2 = GenMov(GenCap(pm1, ply), ply);
    if (pm2 == pm1) return draw_score;    //stalemate
  }
}

  //order moves.  note we don't do a complete sort, just
  //the top handfull which is where cutoffs are most likely.
  hh_fact = 3 * hh_max /2;  //score history moves 0 to 67
  hh = color ? hh_black : hh_white;
  for (pm = pm1; pm < pm2; pm++) {
    move = pm->move;
    if (mv_capro(move)) pm->val = Sex(move);
    else {
      if (move == tree[ply].killer1) pm->val = 80;
      else if (move == tree[ply].killer2) pm->val = 70;
      else pm->val = hh[move & 4095];
    }
    //highest score Sex() can return is about 2 * q_val
    if (move == best_move) pm->val = 3 * q_val;
  }
  SortBubble(pm1, pm2, 5);

  //best move (if we find one) will go in the hash table
  best_move = 0;

  //set end of our moves for next ply and search them
  tree[ply].pm2 = pm2;
  for (pm = pm1; pm < pm2; pm++) {
    move = pm->move;
    Move(move, ply);
    if (depth > 0)
      val = -Search(-beta, -alpha, depth-1, ply+1);
    else
      val = -QSearch(-beta, -alpha, ply+1);
    UnMove(move, ply);
    if (abort_search) return 0;
    if (val > alpha) {  //see what sort of a score we got
      if (val >= beta) {
        //got a cutoff - save our hash and killer move
        HashStore(ply, depth, HF_LOWER, h_threat, val, move);
        UpdateHistory(move, ply, depth);
        return val;
      }
      PVUpdate(ply, move);
      best_move = move;
      alpha = val;
    }
  }
  //done searching moves.  update hash move and killers
  h_type = best_move ? HF_EXACT : HF_UPPER;
  HashStore(ply, depth, h_type, h_threat, alpha, best_move);
  if (best_move) UpdateHistory(best_move, ply, depth);
  return alpha;
}

//====================================================================
//SearchRoot() is similar to Search() but, since things are done a
//bit different at the root it's simpler to have a separate function
//rather than a number of if (ply == 0) statements.
//====================================================================
int SearchRoot(int alpha, int beta, int depth) {
  int val;
  const int ply = 0;
  int move;
  s_move *pm;
  s_move *pm1 = root_list;
  s_move *pm2 = pm1 + root_moves;
  pv_len[ply] = ply;
  tree[ply].key1 = key_1; //update search tree for draw detection

  //set end of our moves for next ply and search them
  tree[ply].pm2 = move_list;
  for (pm = pm1; pm < pm2; pm++) {
    move = pm->move;
    Move(move, ply);
    if (depth > 0)
      val = -Search(-beta, -alpha, depth-1, ply+1);
    else
      val = -QSearch(-beta, -alpha, ply+1);
    UnMove(move, ply);
    if (abort_search) return 0;
    if (val > alpha) {  //see what sort of a score we got
      //new best - put it at the head of the list
      SortReOrder(pm1, pm);
      PVUpdate(ply, move);
      if (val >= beta) return val;  //root fail hi, will have to repeat
      PVDisplay(val, 0);
      alpha = val;
    }
  }
  return alpha;
}

//====================================================================
//Iterate() performs iterative deepening.  It calls Search() with 
//increasing depth until our time is spent.  After each iteration we
//set a window of +/- 1/2 pawn around the score returned.  The 
//narrowed values of alpha and beta produce more cutoffs.  We expect 
//the next iteration score to be inside the window but that may not 
//be the case if the search discovers something that significantly
//changes the score.  If that happens we open the window on the side
//that failed and repeat the search.  We resist the temptation to 
//close the window on the other side which, due to search instability,
//could cause a never ending iteration.
//====================================================================
int Iterate() {
  int val;
  int alpha = -INF;
  int beta = INF;

  SetSearchTime();  //set time target
  if (Draw3Rep(0, TRUE) || (g_ply[0] > 90)) {
    ClearHash();    //clear hash if a draw is lurking
  } else {
    AgeHash();      //otherwise age
  }
  root_score = 0;
  nodes = 0;

  //iterate till our time is spent
  iter = 1;
  // Loop until one of the break conditions is met
  for ( ; ; ) {
    val = SearchRoot(alpha, beta, iter-1);
    if (abort_search) break;
    //see if we got a value inside the window
    if (val <= alpha) {
      alpha = -INF;
      PVDisplay(val, -1);
      continue;             //root fail lo
    }
    if (val >= beta) {
      beta = INF;
      PVDisplay(val, +1);
      continue;             //root fail hi
    }
    //iteration complete, set aspiration window for next iteration
    alpha = val - 50;
    beta = val + 50;
    root_score = val;
    //if 1/2 our time spent we probably can't complete the iteration
    if (Now() - tc_start >  tc_target/2) break;
    iter++;
    if (iter > xb_sd) break;
    if (val > DEAD) break;
  }
  return root_list[0].move;
}



