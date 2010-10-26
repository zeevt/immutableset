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

#include "hash_tools.h"
#include "print_utils.h"

__attribute__ ((unused))
void print_mask(item_t mask)
{
  for (int bit = item_bits - 1; bit >= 0; bit--)
    putchar(mask & (ITEM_T_C(1) << bit) ? '1' : '0');
  putchar('\n');
}

__attribute__ ((const))
item_t apply_simple_hash_function(item_t item, item_t mask)
{
  int obit = 0;
  item_t res = 0;
  for (int ibit = 0; ibit < item_bits; ibit++)
  {
    if (mask & (ITEM_T_C(1) << ibit))
    {
      res |= ((item & (ITEM_T_C(1) << ibit)) >> ibit) << obit;
      obit++;
    }
  }
  return res;
}

void output_mask(FILE* stream, item_t mask, int used_bits)
{
  item_t submask = 0;
  for(int submask_size = 0, obit = 0, ibit = 0; ibit <= item_bits; ibit++)
  {
    if ((!(mask & (ITEM_T_C(1) << ibit))) || (ibit == item_bits))
    {
      if (submask_size)
      {
        fputs("(item & ", stream);
        CALL_PRINT_ITEM(item_t, submask, stream);
        fputs(")", stream);
        int diff = ibit - submask_size - obit;
        if (diff)
          fprintf(stream, " >> %d", diff);
        obit += submask_size;
        if (obit < used_bits)
          fputs(" |" S_EOL, stream);
        submask = 0;
        submask_size = 0;
      }
    }
    else
    {
      submask |= ITEM_T_C(1) << ibit;
      submask_size++;
    }
  }
}

void create_stats(const item_t *array, int size, struct stats_t *stats)
{
  stats->bit_is_on = (int *)calloc(sizeof(int), item_bits);
  stats->correlation = (int *)malloc(sizeof(int) * item_bits * item_bits);
  for (int i = 0; i < size; i++)
  {
    item_t item = array[i];
    for (int bit = 0; bit < item_bits; bit++)
      stats->bit_is_on[bit] += (item & (1 << bit)) >> bit;
    for (int bit1 = 0; bit1 < item_bits - 1; bit1++)
    {
      for (int bit2 = bit1+1; bit2 < item_bits; bit2++)
      {
        int val = ((item & (ITEM_T_C(1) << bit1)) >> bit1) ^ ((item & (ITEM_T_C(1) << bit2)) >> bit2);
        stats->correlation[bit1 * item_bits + bit2] += val;
        stats->correlation[bit2 * item_bits + bit1] += val;
      }
    }
  }
  int target_value = size / 2;
  for (int i = 0; i < item_bits; i++)
    stats->bit_is_on[i] = abs(target_value - stats->bit_is_on[i]);
  /*
  for (int i = 0; i < item_bits; i++)
  {
    printf("%2d:%2d%% ", bit_is_on[i].key, bit_is_on[i].value * 100 / h->size);
    for (int j = 0; j < item_bits; j++)
      if (j != i)
        printf("%2d:%2d%% ", bit_is_on[j].key, correlation[bit_is_on[i].key * item_bits + bit_is_on[j].key] * 100 / h->size);
    putchar('\n');
  }
  */
}

void destroy_stats(struct stats_t *stats)
{
  if (!stats) return;
  if (stats->bit_is_on) { free(stats->bit_is_on); stats->bit_is_on = 0; }
  if (stats->correlation) { free(stats->correlation); stats->correlation = 0; }
}

