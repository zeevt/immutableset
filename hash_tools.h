#ifndef HASH_TOOLS_H_
#define HASH_TOOLS_H_
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

#include <stdio.h>

#include "readonly_set_cfg.h"

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


// http://www-graphics.stanford.edu/~seander/bithacks.html
#if item_bits == 16
__attribute__ ((unused)) __attribute__ ((const))
static int popcount(uint16_t v)
{
  v = v - ((v >> 1) & 0x5555);
  v = (v & 0x3333) + ((v >> 2) & 0x3333);
  v = (v & 0x0F0F) + ((v >> 4) & 0x0F0F);
  return (v & 0xff) + (v >> 8);
}
#elif item_bits == 32
__attribute__ ((unused)) __attribute__ ((const))
static int popcount(uint32_t v)
{
  v = v - ((v >> 1) & 0x55555555);
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
  return (((v + (v >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}
#elif item_bits == 64
__attribute__ ((unused)) __attribute__ ((const))
static int popcount(uint64_t v)
{
  v = v - ((v >> 1) & INT64_C(0x5555555555555555));
  v = (v & INT64_C(0x3333333333333333)) + ((v >> 2) & INT64_C(0x3333333333333333));
  return (((v + (v >> 4)) & INT64_C(0x0F0F0F0F0F0F0F0F)) * INT64_C(0x0101010101010101)) >> 56;
}
#else
#error "sizeof(item_t) must be one of: 2, 4, 8"
#endif

__attribute__ ((unused))
void print_mask(item_t mask);

struct stats_t { int *bit_is_on; int *correlation; };
void create_stats(const item_t *array, int size, struct stats_t *stats);
void destroy_stats(struct stats_t *stats);

__attribute__ ((const))
item_t apply_simple_hash_function(item_t item, item_t mask);

item_t choose_hash_func_mask(const struct stats_t *stats, int needed_bits, int penalty_per_instruction);

void output_mask(FILE* stream, item_t mask, int used_bits);

#endif /* HASH_TOOLS_H_ */
