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
#include <limits.h>

#include "hopscotch.h"
#include "print_utils.h"
#include "hash_tools.h"

struct hopscotch_t
{
  int size;
  int bits_used;
  int neighborhood_size;
  item_t mask;
  item_t *buckets;
  item_t array[C99_FLEXIBLE_ARRAY_MEMBER_LENGTH];
};

struct hopscotch_t* hopscotch_allocate(int size)
{
  struct hopscotch_t* h = (struct hopscotch_t*)malloc(sizeof(struct hopscotch_t) + sizeof(item_t) * size);
  h->size = size;
  h->bits_used = 0;
  h->neighborhood_size = 0;
  h->mask = 0;
  h->buckets = 0;
  return h;
}

void hopscotch_cleanup(struct hopscotch_t* h)
{
  if (!h) return;
  if (h->buckets) free(h->buckets);
  free(h);
}

void hopscotch_fill(struct hopscotch_t* h, iter_t iter, void* opaque)
{
  for (int i = 0; i < h->size; i++) h->array[i] = *(iter(opaque));
  struct stats_t stats;
  create_stats(&h->array[0], h->size, &stats);
  int target_value = h->size / 20;
  h->neighborhood_size = (CACHE_LINE_BYTES / 2) / sizeof(item_t);
  int needed_bits = MS1B(h->size);
  if (h->size > ((1 << needed_bits) + h->neighborhood_size - 1)) needed_bits++;
  for (;;)
  {
    h->mask = choose_hash_func_mask(&stats, needed_bits, target_value);
    h->bits_used = popcount(h->mask);
    int num_buckets = 1 << h->bits_used;
    //printf("size: %d, num_buckets: %d\n", h->size, num_buckets);
    h->buckets = (item_t*)calloc(num_buckets + h->neighborhood_size - 1, sizeof(item_t));
    int *metadata = (int*)calloc(num_buckets + h->neighborhood_size - 1, sizeof(int));
    for (int item_being_placed = 0; item_being_placed < h->size; item_being_placed++)
    {
      //printf("begin trying to place item #%d: %08X\n", item_being_placed, h->array[item_being_placed]);
      int key = (int)apply_simple_hash_function(h->array[item_being_placed], h->mask);
      //printf("key is: %d\n", key);
      int probe = key;
      int neighborhood_i = 0;
      for ( ; neighborhood_i < h->neighborhood_size; probe++, neighborhood_i++)
      {
        if (h->buckets[probe] == 0)
        {
        place_in_keys_neighborhood:
          //printf("placing into bucket %d, as item %d of key %d\n", probe, neighborhood_i, key);
          h->buckets[probe] = h->array[item_being_placed];
          metadata[key] |= 1 << neighborhood_i;
          goto has_placed;
        }
      }
      //printf("searching for empty bucket away from neighborhood\n");
      for ( ; probe < num_buckets + h->neighborhood_size; probe++)
        if (h->buckets[probe] == 0)
          goto has_found_empty_bucket;
      goto failed_to_find_space;
    has_found_empty_bucket:
      //printf("found empty bucket in #%d\n", probe);
      do
      {
        for (int probe_back = 1; probe_back < h->neighborhood_size; probe_back++)
        {
          if (!metadata[probe - probe_back]) continue;
          for (int thing_to_relocate = 0; thing_to_relocate < probe_back; thing_to_relocate++)
          {
            if (metadata[probe - probe_back] & (1 << thing_to_relocate))
            {
              //printf("moving item %08X that was in bucket #%d (as item %d of key #%d) into bucket #%d (as item %d of key #%d)\n",
              //h->buckets[probe - probe_back + thing_to_relocate],
              //probe - probe_back + thing_to_relocate, thing_to_relocate, probe - probe_back,
              //probe, probe_back, probe - probe_back);
              h->buckets[probe] = h->buckets[probe - probe_back + thing_to_relocate];
              metadata[probe - probe_back] &= ~(1 << thing_to_relocate);
              metadata[probe - probe_back] |= (1 << probe_back);
              probe = probe - probe_back + thing_to_relocate;
              goto next_hop;
            }
          }
        }
        goto failed_to_find_space;
      next_hop:
        ;
      }
      while (probe - key >= h->neighborhood_size);
      neighborhood_i = probe - key;
      goto place_in_keys_neighborhood;
    has_placed:
      ;
    }
    free(metadata);
    destroy_stats(&stats);
    return;
  failed_to_find_space:
    free(metadata);
    free(h->buckets);
    needed_bits++;
  }
}

void hopscotch_generate_c(const struct hopscotch_t* h, FILE* stream)
{
  int num_buckets = 1 << h->bits_used;
  fputs(
  GENERATED_FILE_PROLOGUE
  "#define item_t " QUOTEME(item_t) "\n"
  "static const item_t array[] =" S_EOL "{\n", stream);
  CALL_PRINT_ARRAY(item_t, stream, h->buckets, num_buckets + h->neighborhood_size - 1);
  fputs(S_EOL "};\n"
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
    "item_t key =" S_EOL, stream);
  output_mask(stream, h->mask, h->bits_used);
  fputs(
    ";\n"
    "const item_t *p = &array[key];\n"
    "#define CHECK() if (*p == item) return p; else ++p;\n"
  , stream);
  for (int i = 0; i < h->neighborhood_size; i++)
    fputs("CHECK()" S_EOL, stream);
  fputs(
    "return 0;" S_EOL
  "}" S_EOL
  , stream);
}
