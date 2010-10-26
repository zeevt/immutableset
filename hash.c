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

#include "hash.h"
#include "print_utils.h"
#include "hash_tools.h"

struct hash_t
{
  int size;
  int bits_used;
  item_t mask;
  int *buckets;
  item_t array[C99_FLEXIBLE_ARRAY_MEMBER_LENGTH];
};

struct hash_t* hash_allocate(int size)
{
  struct hash_t* h = (struct hash_t*)malloc(sizeof(struct hash_t) + sizeof(item_t) * size);
  h->size = size;
  h->bits_used = 0;
  h->mask = 0;
  h->buckets = 0;
  return h;
}

void hash_cleanup(struct hash_t* h)
{
  if (!h) return;
  if (h->buckets) free(h->buckets);
  free(h);
}

void hash_fill(struct hash_t* h, iter_t iter, void* opaque)
{
  for (int i = 0; i < h->size; i++) h->array[i] = *(iter(opaque));
  struct stats_t stats;
  create_stats(&h->array[0], h->size, &stats);
  int needed_bits = MS1B(h->size) - 1;
  int target_value = h->size / 20;
  h->mask = choose_hash_func_mask(&stats, needed_bits, target_value);
  h->bits_used = popcount(h->mask);
  item_t num_buckets = 1 << h->bits_used;
  destroy_stats(&stats);
  item_t *keys = (item_t *)malloc(sizeof(item_t) * h->size);
  for (int i = 0; i < h->size; i++)
    keys[i] = apply_simple_hash_function(h->array[i], h->mask);
  h->buckets = (int*)malloc(sizeof(int) * (num_buckets + 1));
  h->buckets[num_buckets] = h->size;
  int curr_o = 0, scan = 0;
  item_t curr_key = 0;
  h->buckets[curr_key] = 0;
  while (curr_key < num_buckets)
  {
    while ((scan < h->size) && (keys[scan] != curr_key))
      scan++;
    if (h->size == scan)
    {
      scan = curr_o;
      curr_key++;
      h->buckets[curr_key] = curr_o;
      continue;
    }
    item_t temp = keys[scan];
    keys[scan] = keys[curr_o];
    keys[curr_o] = temp;
    temp = h->array[scan];
    h->array[scan] = h->array[curr_o];
    h->array[curr_o] = temp;
    curr_o++;
    scan++;
  }
  free(keys);
  /*
  #define ARRLEN(a) ((int)(sizeof(a)/sizeof(*a)))
  printf("size: %d, num buckets: %d\n", h->size, num_buckets);
  int lengths[32];
  memset(lengths, 0, sizeof(lengths));
  for (item_t i = 0; i < num_buckets; i++)
  {
    int curr = h->buckets[i+1] - h->buckets[i];
    if (curr > (ARRLEN(lengths) - 1))
      lengths[ARRLEN(lengths) - 1]++;
    else
      lengths[curr]++;
  }
  for (int i = 0; i < ARRLEN(lengths); i++)
    printf("%2d: %3d\n", i, lengths[i]);
  int max_length = ARRLEN(lengths) - 1;
  for (; !lengths[max_length]; max_length--) ;
  printf("max bucket chain length: %d\n", max_length);
  */
}

DEFINE_PRINT_FUNCS(int, UNUSED static)

void hash_generate_c(const struct hash_t* h, FILE* stream)
{
  fputs(
  GENERATED_FILE_PROLOGUE
  "#define item_t " QUOTEME(item_t) "\n"
  "static const item_t array[] =" S_EOL "{\n", stream);
  CALL_PRINT_ARRAY(item_t, stream, h->array, h->size);
  fputs(S_EOL "};\n", stream);
  int num_buckets = 1 << h->bits_used;
  int bucket_bits = MS1B(h->size), bucket_bytes;
  if (bucket_bits <= 8)
    bucket_bytes = 1;
  else if (bucket_bits <= 16)
    bucket_bytes = 2;
  else
    bucket_bytes = 4;
  fprintf(stream, "static const uint%d_t buckets[] = {" S_EOL, bucket_bytes * CHAR_BIT);
  print_int_array(bucket_bytes, stream, h->buckets, num_buckets + 1);
  fputs(
  "};\n"
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
    "item_t key =" S_EOL, stream);
  output_mask(stream, h->mask, h->bits_used);
  fputs(
    ";" S_EOL
    "for (int low = buckets[key], high = buckets[key+1]; low < high; low++)" S_EOL
      "if (array[low] == item)" S_EOL
        "return &array[low];" S_EOL
    "return 0;" S_EOL
  "}" S_EOL, stream);
}
