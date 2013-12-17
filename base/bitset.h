#ifndef _BASE_BITSET_H
#define _BASE_BITSET_H

#include <limits.h> /* for CHAR_BIT */

#define BITMASK(b)      (1 << ((b) % CHAR_BIT))
#define BITSLOT(b)      ((b) / CHAR_BIT)
#define BITSET(a, b)    ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b)  ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b)   ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb)   ((nb + CHAR_BIT - 1) / CHAR_BIT)
/*
Here are some usage examples. To declare an ``array'' of 47 bits:
char bitarray[BITNSLOTS(47)];

To set the 23rd bit:
BITSET(bitarray, 23);

To test the 35th bit:
if(BITTEST(bitarray, 35)) ...

To compute the union of two bit arrays and place it in a third array (with all three arrays declared as above):
for(i = 0; i < BITNSLOTS(47); i++)
  array3[i] = array1[i] | array2[i];

To compute the intersection, use & instead of |.
*/

#endif