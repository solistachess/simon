//system.h by Dan Honeycutt.  This software is public domain.
//You may use this software as you wish.  There is no warranty.
#ifndef SYSTEM_H
#define SYSTEM_H

/*********************************************************************
File includes system dependent functions.  Only the WIN32 versions
are tested.  If you take simon to another platform you will have to
straighten out whtever doesn't work yourself
*********************************************************************/

#ifdef WIN32
  typedef unsigned __int64 U64;   //bitboard data type
  #include <windows.h>
#else
  typedef unsigned long long U64;
  #include <sys/time.h>
#endif

//=====================================================================
//Now() returns time in ms.
//=====================================================================
__inline int Now() {
#ifdef WIN32
  return GetTickCount();
#else
  struct timeval  tv;
  gettimeofday( &tv, NULL );
  return (int) (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

//====================================================================
//FirstBit(), LastBit(), and BitCount() locate and count bits in a 64
//bit (bitboard) value.  for example the classic center squares for
//white (d4 and e4) have the (rather meaningless) decimal value of
//402653184.  things clear up some if we look at the binary value:
//   0000000000000000000000000000000000011000000000000000000000000000
//   ^                                   ^  ^       ^       ^       ^
//   h8                                  d4 a4      a3      a2      a1
//sq 63                                  27 24      16      8       0
//FirstBit(402653184) returns 27 (square d4), LastBit(402653184)
//returns 28 (square e4) and BitCount(402653184) returns 2.

//BitCount() uses the fact that x & (x - 1) clears the low bit in
//x.  It simply resets 1 bit at a time and counts them till gone
//====================================================================

#ifdef WIN32

//keep compiler from barking about no return value
#pragma warning(disable : 4035)

__forceinline int FirstBit(U64 val)  {
  __asm {
        bsf eax, dword ptr val
        jnz done
        bsf eax, dword ptr val + 4
        add eax, 32
done:
  }
}

__forceinline int LastBit(U64 val) {
  __asm {
        bsr eax, dword ptr val + 4
        jnz a1
        bsr eax, dword ptr val
        jnz done
        mov eax, 32
a1:     add eax, 32
done:
  }
}

__forceinline int BitCount(U64 val) {
  __asm {
        xor eax, eax                //zero count
        mov ecx, dword ptr val      //low dword
        test ecx, ecx               //any bits ?
        jz hi                       //no jump
loop1:  lea edx, [ecx - 1]
        inc eax                     //increment bit count
        and ecx, edx                //clear bit
        jnz loop1                   //loop if still bits
hi:     mov ecx, dword ptr val + 4  //repeat for hi dword
        test ecx, ecx
        jz done
loop2:  lea edx,[ecx - 1]
        inc eax
        and ecx, edx
        jnz loop2
done:
  }
}
#pragma warning(default : 4035)

#else   //ifdef WIN32

//====================================================================
//non assembly versions.  BitCount() is OK but the C versions First/
//LastBit() is too slow.  to simon FirstBit() and LastBit() are the 
//same so there is only one function for both.  To find things like
//the most advanced or backward pawn you will need the functionality
//of both.
//====================================================================
#define FirstBit LastBit

__inline int LastBit(U64 val) {
  int i1, b2 = 0;
  for (i1 = 32; i1; i1 >>= 1) {
    if (val >= (U64)1 << i1) {
      b2 += i1;
      val >>= i1;
    }
  }
  return (b2);
}

__inline int BitCount(U64 b) {
  int i;
  for (i = 0; b; i++, b &= (b - 1));
  return i;
}

#endif  //ifdef WIN32

#endif  //ifndef SYSTEM_H
