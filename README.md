# simon
For archival purposes: Dan Honeycutt's "simon", a chessengine from 2004


Introduction
============

Simon is a Winboard compatible chess engine.  It is a fairly simple program, 
in particular the search and evaluation are very basic.  However, it is a 
fully functional engine using a bitboard infrastructure including rotated bitboard 
attack calculations.

Simon is for people who have an interest in chess programming, in particular those 
interested in the bitboard board representation, but see the task of just getting 
started as a daunting one.  Simon provides a starting point.  I've tried to keep 
the code clean and clear and to comment extensively.

There are other relatively simple open source programs one could use as a starting 
point such as TSCP, Gerbil and Faile.  While open source, these programs are 
copyrighted and you must conform to the copyright conditions before you can use them.  
Copyright issues have been the source of many disputes, hard feelings and 
misunderstandings.

To avoid these issues, Simon is not copyrighted.  The software is public domain.  Those 
who write a chess program using another program as a starting point would one day like 
to call it their own.  With Simon this is not an issue.  It's yours from day 1.

Since there is no copyright I can not impose conditions, but there is one condition I 
do request:  that you not enter Simon in any competition until you have achieved 
significant improvement.  Specifically, you should improve Simon to the point where it 
is as good or better than the aforementioned programs (TSCP, Gerbil and Faile).

Origin
======

Simon was derived from my program Bruja.  The board structure and bitboard move 
generation functions are the same as Bruja's.  The search and evaluation (the heart of 
the program) and most of the whistles and bells have been removed or replaced by very 
basic versions.  After some self debate I decided to leave Bruja's hash table functions 
only because these are such a pain in the ass to get to work right (it took me three 
complete rewrites and I still don't guarantee that they are 100 percent correct).

Acknowledgements
================

Since it is derived from Bruja I would be remiss not to extend the same acknowledgement 
and appreciation that you will find in Bruja's readme file.  Special thanks in particular 
go to Bruce Moreland (Gerbil), and Adrien Regimbald (Faile), Carlos del Cacho (Pepito) 
and Dr. Robert Hyatt (Crafty).

Program Overview
================

If you are new to chess programming I recommend you start with Bruce Moreland's chess 
programming pages (http://www.seanet.com/~brucemo/topics/topics.htm).  There you will 
find the basic alpha-beta search function:

int AlphaBeta(int depth, int alpha, int beta) {
    if (depth == 0) return Evaluate();
    GenerateLegalMoves();
    while (MovesLeft()) {
        MakeNextMove();
        val = -AlphaBeta(depth - 1, -beta, -alpha);
        UnmakeMove();
        if (val >= beta) return beta;
        if (val > alpha) alpha = val;
    }
    return alpha;
}

Bruce explains how it works so I won't repeat that.  To support AlphaBeta() we need the 
following functions:

Evaluate() - provides a score for the position.
GenerateLegalMoves() - generates moves for the current position.
MakeNextMove() - plays a move.
UnmakeMove() - unmakes a move, inverse of MakeNextMove()

A chess program, at the most basic level, consists of just 5 functions: the above 4 
support functions plus AlphaBeta() itself.  (MovesLeft() doesn't count - it's just 
something to tell us when we reach the end of the moves we generated).

Simple enough.  Five functions to score the position, generate moves, make and unmake 
the moves and conduct the search.  In Simon you will find these 5 functions (with 
somewhat different names) in the files:

1. eval.cpp
2. gen.cpp
3. move.cpp
4. search.cpp

The search will produce our moves, but we also need something to manage the game - my 
turn, your turn, I move here, you move there sort of thing.  In Simon this is handled by 
the function Play() in the file:

5. play.cpp

Before we can play we need something to set up the chessboard:

6. setboard.cpp

The rotated bitboards which we use to calculate attacks tell us important things like 
if we are in check so they get a file:

7. attack.cpp

The aforementioned pain-in-the-butt hash tables also get a file:

8. hash.cpp

Every program needs some variables which you will find in:

9. data.cpp

There are always assorted miscellaneous functions:

10. utility.cpp

And, finally the program start and initialization:

11. main.cpp

These are the 11 files (and where the fit in the big picture) that make up Simon.  
There are several header files (the main one being chess.h) with defines, function 
prototypes and the like.  Each file has a comment block with a more detailed 
explanation of its purpose.

Enjoy
=====
I hope you find Simon useful and I wish you success in your chess programming.  It 
can be frustrating but it's also a lot of fun.  I welcome all comments, criticisms 
and suggestions which you can email to me at dhoneycutt@jjg.com.

Dec 2004
Dan Honeycutt.


Release History
===============
v1.0 12/19/04 Initial Release

v1.1 1/27/05 Fixed sign reversal of mate scores.  Thanks to Dann Corbit for 
finding this bug.
