/*
Copyright (c) 2010 Zeev Tarantov (zeev.tarantov@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include "inttypes.h"
#define C99_FLEXIBLE_ARRAY_MEMBER_LENGTH 1 /* Microsoft doesn't support C99 */
#else
#include <inttypes.h>
#define C99_FLEXIBLE_ARRAY_MEMBER_LENGTH 
#endif
#include <limits.h>
#include <string.h>
#ifdef WIN32
#include <intrin.h>
static uint32_t winnt_bsr(uint32_t v)
{
  unsigned long res;
  _BitScanReverse(&res, v);
  return (uint32_t)res;
}
#define MS1B(n) winnt_bsr(n)
#else
#define MS1B(n) (sizeof(int)*CHAR_BIT - __builtin_clz(n))
#endif

#include "ghash.h"
#include "print_utils.h"

struct ghash_t
{
  int size;
  item_t mask;
  item_t array[C99_FLEXIBLE_ARRAY_MEMBER_LENGTH];
};

struct ghash_t* ghash_allocate(int size)
{
  int needed_size = 1 << (MS1B(size) + 1);
//  printf("size: %d, needed size: %d\n", size, needed_size);
  struct ghash_t* h = (struct ghash_t*)malloc(sizeof(int) + sizeof(item_t) + sizeof(item_t) * needed_size);
  h->size = size;
  h->mask = needed_size - 1;
  memset(h->array, 0, sizeof(item_t) * needed_size);
  return h;
}

static uint32_t smear(uint32_t hashCode)
{
  hashCode ^= (hashCode >> 20) ^ (hashCode >> 12);
  return hashCode ^ (hashCode >> 7) ^ (hashCode >> 4);
}

void ghash_fill(struct ghash_t* h, iter_t iter, void* opaque)
{
  for (int i = 0; i < h->size; i++)
  {
    const item_t curr = *(iter(opaque));
    int index = smear(curr) & h->mask;
    while (h->array[index])
      index = (index + 1) & h->mask;
    h->array[index] = curr;
  }
}

void ghash_generate_c(const struct ghash_t* h, FILE* stream)
{
  fputs(
  GENERATED_FILE_PROLOGUE
  "#define item_t " QUOTEME(item_t) "\n"
  "static const item_t array[] =" S_EOL "{\n", stream);
  CALL_PRINT_ARRAY(item_t, stream, h->array, h->mask + 1);
  fputs(S_EOL "};\n"
  "#define MASK (sizeof(array)/sizeof(item_t) - 1)\n"
  "static uint32_t smear(uint32_t hashCode)" S_EOL
  "{" S_EOL
    "hashCode ^= (hashCode >> 20) ^ (hashCode >> 12);" S_EOL
    "return hashCode ^ (hashCode >> 7) ^ (hashCode >> 4);" S_EOL
  "}" S_EOL
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
    "int i = smear(item) & MASK;" S_EOL
    "while (array[i])" S_EOL
    "{" S_EOL
      "if (item == array[i]) return &array[i];" S_EOL
      "i = (i + 1) & MASK;" S_EOL
    "}" S_EOL
    "return 0;" S_EOL
  "}" S_EOL, stream);
}
