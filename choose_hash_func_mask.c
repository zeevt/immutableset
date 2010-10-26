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
#include <stdint.h>
#include <limits.h>
#include "readonly_set_cfg.h"
#include "hash_tools.h"

extern int slogaemie(unsigned char* output, int size, int sum);
extern int next_permutation(unsigned char *first, unsigned char *last);

static int calc_penalty(item_t mask, const int *bit_is_on)
{
  int penalty = 0;
  for (int i = 0; i < item_bits; i++)
    if (mask & (ITEM_T_C(1) << i))
      penalty += bit_is_on[i];
  return penalty;
}

static void position_submasks(item_t *submasks, unsigned char *submask_start, const unsigned char *submask_length, int begin, int num_submasks, int next_start)
{
  for (int i = begin; i < num_submasks; i++)
  {
    submasks[i] = 1;
    for (int bit = 1; bit < submask_length[i]; bit++)
      submasks[i] = (submasks[i] << 1) + 1;
    submasks[i] <<= next_start;
    submask_start[i] = next_start;
    next_start += submask_length[i] + 1;
  }
}

item_t choose_hash_func_mask(const struct stats_t *stats, int needed_bits, int penalty_per_instruction)
{
  /*
  generate candidate mask
  penalty = sum(abs(value - target_value) for value in masked bit)
  execution_time = sum(1 for submask starting at LSB, 3 for each submask starting not as LSB for each submask)
  cost = formula that relates penalty to execution_time with some coefficient
  choose the candidate mask with the lowest cost.
  for debug version, keep all candidates in list, sort it.
  for real, just keep the best so far (min_cost) and compare each new one to it.
  for version two, penalize also using correlation data.
  */
  
  item_t best_mask = 0;
  int best_penalty = INT_MAX;
  int best_time = 0;
  int max_possible_submasks = (needed_bits <= (item_bits / 2)) ? needed_bits : (item_bits - needed_bits + 1);
  void* mem = malloc((sizeof(item_t) + 2) * max_possible_submasks);
  item_t *submask = (item_t *)mem;
  unsigned char *submask_length = (unsigned char *)mem + (max_possible_submasks * sizeof(item_t));
  unsigned char *submask_start = submask_length + max_possible_submasks;
  for (int num_submasks = 1; num_submasks <= max_possible_submasks; num_submasks++)
  {
    // loop over possible ways to get a sum of needed_bits from num_submasks natural numbers
    // for each, loop over permutations with repetition of ways to order those.
    // for each array of submask lengths, shift submasks one by one towards the other end.
    int time = 3 * num_submasks - 2;
    if ((time - best_time) * penalty_per_instruction > best_penalty) goto end;
    for (int i = 0; i < num_submasks; i++) submask_length[i] = 0;
    while (slogaemie(submask_length, num_submasks, needed_bits))
    {
      do
      {
        position_submasks(submask, submask_start, submask_length, 0, num_submasks, 0);
        time = 3 * num_submasks - 2;
        for (;;)
        {
          item_t mask = 0;
          for (int i = 0; i < num_submasks; i++) mask |= submask[i];
          //print_mask(mask);
          int penalty = calc_penalty(mask, stats->bit_is_on);
          if ((best_penalty - penalty) > (time - best_time) * penalty_per_instruction)
          {
            best_mask = mask;
            best_penalty = penalty;
            best_time = time;
          }
          /* shift to next position */
          int submask_to_move = num_submasks - 1;
          for (int next_end = item_bits; submask_to_move >= 0; submask_to_move--)
          {
            if (submask_start[submask_to_move] + submask_length[submask_to_move] < next_end)
            {
              if ((submask_to_move == 0) && (submask_start[submask_to_move] == 0)) time++;
              submask_start[submask_to_move]++;
              submask[submask_to_move] <<= 1;
              goto has_shifted_a_submask;
            }
            next_end -= submask_length[submask_to_move] + 1;
          }
          break;
          has_shifted_a_submask:
          position_submasks(submask, submask_start, submask_length,
                            submask_to_move + 1, num_submasks,
                            submask_start[submask_to_move] + submask_length[submask_to_move] + 1);
        }
      }
      while (next_permutation(submask_length, submask_length + num_submasks));
    }
    if (best_penalty < penalty_per_instruction) goto end;
  }
end:
  free(mem);
  return best_mask;
}

#ifdef TEST

#include <stdio.h>
#define ARRSIZE(a) (sizeof(a)/sizeof(*a))

static const struct { int values[item_bits]; int size; int needed_bits; item_t expected; } tests[] = {
{{500,494,492,493,492,491,490,489,505,504,506,502,501,500,510,488,404,507,521,498,484,508,506,490,512,513,514,490,495,444,535,500}, 1000, 9, 0x000001FF},
{{500,494,492,493,492,491,0,489,505,504,506,502,501,0,510,488,404,507,521,498,484,0,506,490,512,513,514,490,495,484,515,500}, 1000, 9, 0xFF800000},
{{500,494,492,493,492,491,490,0,505,504,506,502,501,500,510,0,404,507,521,498,484,508,506,0,512,513,514,490,495,444,535,0}, 1000, 9, 0x00003F07},
{{500,494,492,493,0,491,490,489,505,999,506,0,501,0,510,0,404,0,521,498,484,0,506,490,0,513,514,0,495,484,515,500}, 1000, 9, 0x800001EF},
{{0,491,0,492,0,493,0,494,0,495,0,496,0,487,0,498,0,499,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1000, 9, 0x00002AAAA},
};

int main()
{
  int bit_is_on[item_bits];
  for (unsigned currTest = 0; currTest < ARRSIZE(tests); currTest++)
  {
    int target_value = tests[currTest].size / 2;
    for (int i = 0; i < item_bits; i++)
      bit_is_on[i] = abs(target_value - tests[currTest].values[i]);
    struct stats_t stats = { &bit_is_on[0], NULL };
    const item_t result = choose_hash_func_mask(&stats, tests[currTest].needed_bits, target_value / 10);
    if (result == tests[currTest].expected)
    {
      printf("passed!\n");
    }
    else
    {
      printf("failed!\n");
      for (int i = item_bits - 1; i >= 0; i--)
        printf("%d ", bit_is_on[i]);
      printf("\nExpected: 0x%08X, Got: 0x%08X\n", tests[currTest].expected, result);
    }
  }
  return 0;
}

#endif /* TEST */
