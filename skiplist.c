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
#include <string.h>
#include <assert.h>

#include "skiplist.h"
#include "print_utils.h"

static inline int roundup(int i, int n)
{
  if (!i) return n;
  return (i / n + (((i % n) == 0) ? 0 : 1)) * n;
}

#define BUCKET_ITEMS     (CACHE_LINE_BYTES / sizeof(item_t))

static const int total_capacity[] =
{
  BUCKET_ITEMS,
  (BUCKET_ITEMS + 1) * BUCKET_ITEMS,
  ((BUCKET_ITEMS + 1) * BUCKET_ITEMS) * BUCKET_ITEMS,
  (((BUCKET_ITEMS + 1) * BUCKET_ITEMS) * BUCKET_ITEMS) * BUCKET_ITEMS
};

#define LEVELS_SUPPORTED  (sizeof(total_capacity) / sizeof(*total_capacity))

struct skiplist_t
{
  item_t* array;
  int     size;
  int     max_level;
  int     total_length;
  int     offsets[C99_FLEXIBLE_ARRAY_MEMBER_LENGTH];
};

static int max_level_required(int size)
{
  for (int i = 0; i < (int)(sizeof(total_capacity)/sizeof(*total_capacity)); i++)
    if (size <= total_capacity[i])
      return i;
  return -1;
}

UNUSED static int max_offset(const struct skiplist_t* sl, int level)
{
  return (level != sl->max_level) ? sl->offsets[level+1] : sl->total_length;
}

UNUSED static void skiplist_debug_print(const struct skiplist_t* sl)
{
  printf("size: %d, max_level: %d, total length: %d\n", sl->size, sl->max_level, sl->total_length);
  for (int i = 0; i <= sl->max_level; i++)
  {
    printf("level: %d, begin: %d\n", i, sl->offsets[i]);
  }
}

UNUSED static void skiplist_print(const struct skiplist_t* sl)
{
  skiplist_debug_print(sl);
  for (int level = 0; level <= sl->max_level; level++)
  {
    printf("level %d:\n", level);
    for (int curr = sl->offsets[level], max = max_offset(sl, level), i = 1; curr < max; i++, curr++)
    {
      printf("%" PR_item_t, sl->array[curr]);
      if (i % BUCKET_ITEMS == 0)
        printf("| ");
    }
    printf("\n");
  }
}

void skiplist_generate_c(const struct skiplist_t* sl, FILE* stream)
{
  fputs(
  GENERATED_FILE_PROLOGUE
  "#include <assert.h>\n", stream);
  fprintf(stream, "#define CACHE_LINE_BYTES %d\n", CACHE_LINE_BYTES);
  fputs("#define item_t " QUOTEME(item_t) "\n", stream);
  if (sl->max_level)
  {
    fputs("static const int offsets[] = { ", stream);
    for (int i = 1; i <= sl->max_level; i++)
    {
      fprintf(stream, "%d", sl->offsets[i]);
      if (sl->max_level != i)
        fputs(", ", stream);
    }
    fputs(" };\n", stream);
  }
  else
  {
    fputs("static const int offsets[0];\n", stream);
  }
  fputs(
    "#ifdef WIN32\n"
    "#define ALIGN(n) __declspec(align(n))\n"
    "#else\n"
    "#define ALIGN(n) __attribute__ ((aligned(n)))\n"
    "#endif\n"
    "ALIGN(CACHE_LINE_BYTES) static const item_t array[] =" S_EOL "{\n"
  , stream);
  CALL_PRINT_ARRAY(item_t, stream, sl->array, sl->total_length);
  fputs(
  S_EOL "};\n"
  "#define BUCKET_ITEMS     (CACHE_LINE_BYTES / sizeof(item_t))\n"
  "#define ARRLEN(a)        (sizeof(a)/sizeof(*a))\n"
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
    "assert(0 == ((intptr_t)&array[0] % CACHE_LINE_BYTES));\n"
    "#if defined(__GNUC_MINOR__) && (__GNUC_MINOR__ > 3)\n"
    "unsigned level = 0, offset = 0, breadcrumbs = 0;" S_EOL
    "while (level < ARRLEN(offsets))" S_EOL
    "{" S_EOL
      "for (unsigned i = 0; i < BUCKET_ITEMS; i++)" S_EOL
      "{" S_EOL
        "if (item == array[ offset + i ])" S_EOL
          "return &array[ offset + i ];" S_EOL
        "if (item < array[ offset + i ])" S_EOL
        "{" S_EOL
          "breadcrumbs = (breadcrumbs | i) * BUCKET_ITEMS;" S_EOL
          "offset = offsets[level++] + breadcrumbs;" S_EOL
          "goto again;" S_EOL
        "}" S_EOL
      "}" S_EOL
      "return 0;" S_EOL
      "again:;" S_EOL
    "}" S_EOL
    "if (offset >= ARRLEN(array))" S_EOL
      "return 0;" S_EOL
    "for (unsigned i = 0; i < BUCKET_ITEMS; i++)" S_EOL
    "{" S_EOL
      "assert((offset + i) < ARRLEN(array));" S_EOL
      "if (item == array[ offset + i ])" S_EOL
        "return &array[ offset + i ];" S_EOL
    "}\n"
    "#else\n"
    "unsigned offset = 0, breadcrumbs = 0;" S_EOL
    "#define CHECK_EQ(i) \\\n"
      "if (item == array[ offset + i ]) \\\n"
          "return &array[ offset + i ];\n"
    "#define CHECK_EQ_AND_LT(next_level, i) \\\n"
      "CHECK_EQ(i) \\\n"
      "if (item < array[ offset + i ]) \\\n"
      "{ \\\n"
        "breadcrumbs = (breadcrumbs | i) * BUCKET_ITEMS; \\\n"
        "offset = offsets[next_level - 1] + breadcrumbs; \\\n"
        "goto level##next_level; \\\n"
      "}\n"
  , stream);
  for (int level = 0; level < sl->max_level; level++)
  {
    if (level)
      fprintf(stream, "level%d:" S_EOL, level);
    for (unsigned i = 0; i < BUCKET_ITEMS; i++)
      fprintf(stream, "CHECK_EQ_AND_LT(%d, %d)" S_EOL, level + 1, i);
    fputs("return 0;" S_EOL, stream);
  }
  fprintf(stream,
    "level%d:" S_EOL
    "if (offset >= ARRLEN(array))" S_EOL
      "return 0;" S_EOL, sl->max_level);
  for (unsigned i = 0; i < BUCKET_ITEMS; i++)
    fprintf(stream, "CHECK_EQ(%d)" S_EOL, i);
  fputs(
    "\n#endif /* __GNUC__ */\n"
    "return 0;" S_EOL
  "}" S_EOL, stream);
}

void skiplist_cleanup(struct skiplist_t* sl)
{
  if (!sl) return;
  if (sl->array)
    free(sl->array);
  free(sl);
}

static void skiplist_calc_space(struct skiplist_t* sl)
{
  int length[LEVELS_SUPPORTED];
  int remaining = sl->size;
  for (int i = 0; i < sl->max_level; i++)
  {
    length[i] = remaining / (total_capacity[sl->max_level - i - 1] + 1) + 1;
    remaining -= length[i];
  }
  length[sl->max_level] = remaining + 1;
  sl->offsets[0] = 0;
  for (int i = 1; i <= sl->max_level; i++)
  {
    sl->offsets[i] = sl->offsets[i-1] + roundup(length[i-1], BUCKET_ITEMS);
  }
  sl->total_length = sl->offsets[sl->max_level] + roundup(length[sl->max_level], BUCKET_ITEMS);
}

struct skiplist_t* skiplist_allocate(int size)
{
  struct skiplist_t *sl;
  int max_level = max_level_required(size);
  if (-1 == max_level)
  {
    fprintf(stderr, "data size too big\n");
    goto end;
  }
  
  sl = (struct skiplist_t*)malloc(sizeof(struct skiplist_t) + sizeof(int) * (max_level + 1));
  sl->array = NULL;
  sl->size = size;
  sl->max_level = max_level;
  
  memset(sl->offsets, 0, sizeof(int) * (sl->max_level + 1));
  
  skiplist_calc_space(sl);
//skiplist_debug_print(sl);
  
  sl->array = (item_t*)calloc(sl->total_length, sizeof(item_t));
  if (!sl->array)
    goto cleanup;
  return sl;
cleanup:
  free(sl);
end:
  return NULL;
}

struct filler_state_t
{
  item_t* curr_in_array;
  int curr_in_level;
};

static void skiplist_place_item(struct skiplist_t* sl, struct filler_state_t* filler_state, item_t* item)
{
  int level = sl->max_level;
  while (BUCKET_ITEMS == filler_state[level].curr_in_level)
    level--;
  assert((filler_state[level].curr_in_array - &sl->array[0]) < sl->total_length);
  *(filler_state[level].curr_in_array++) = *item;
  filler_state[level].curr_in_level++;
  if (BUCKET_ITEMS != filler_state[level].curr_in_level)
    for (int i = sl->max_level; i > level; i--)
      filler_state[i].curr_in_level = 0;
}

void skiplist_fill(struct skiplist_t* sl, iter_t iter, void* opaque)
{
  struct filler_state_t filler_state[LEVELS_SUPPORTED];
  for (int i = 0; i <= sl->max_level; i++)
  {
    filler_state[i].curr_in_array = &sl->array[sl->offsets[i]];
    filler_state[i].curr_in_level = 0;
  }
  int source_i = 1;
  for ( ; source_i <= sl->size - sl->max_level; source_i++)
  {
    skiplist_place_item(sl, filler_state, iter(opaque));
  }
//skiplist_debug_print(sl);
//skiplist_print(sl);
//for (int i = 0; i <= sl->max_level; i++) printf("level: %d, curr_in_level: %d\n", i, filler_state[i].curr_in_level);
  int level = sl->max_level - 1;
  while (source_i <= sl->size)
  {
    int skipped_level = 0;
    while (BUCKET_ITEMS == filler_state[level].curr_in_level)
    {
      skipped_level = 1;
      level--;
    }
    *(filler_state[level].curr_in_array++) = *(iter(opaque));
    filler_state[level].curr_in_level++;
    if (!skipped_level && level)
      level--;
    source_i++;
  }
}
