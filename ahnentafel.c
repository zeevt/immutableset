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
#include <string.h>
#ifdef WIN32
#include "inttypes.h"
#define C99_FLEXIBLE_ARRAY_MEMBER_LENGTH 1 /* Microsoft doesn't support C99 */
#else
#include <inttypes.h>
#define C99_FLEXIBLE_ARRAY_MEMBER_LENGTH 
#endif

#include "ahnentafel.h"
#include "print_utils.h"

struct ahnentafel_t
{
  int size;
  int bst_size;
  item_t *bst;
  item_t array[C99_FLEXIBLE_ARRAY_MEMBER_LENGTH];
};

struct ahnentafel_t* ahnentafel_allocate(int size)
{
  struct ahnentafel_t* bs = (struct ahnentafel_t*)malloc(sizeof(int)*2 + sizeof(item_t *) + sizeof(item_t) * size);
  bs->size = size;
  bs->bst_size = 0;
  bs->bst = 0;
  return bs;
}

void ahnentafel_cleanup(struct ahnentafel_t* bs)
{
  if (!bs) return;
  if (bs->bst) free(bs->bst);
  free(bs);
}

static void fill_bst(struct ahnentafel_t* bs, int left, int right, int n)
{
  int i = left + (right - left) / 2;
  bs->bst[n] = bs->array[i];
  if (left != i)
    fill_bst(bs, left, i - 1, 2 * n + 1);
  if (right != i)
    fill_bst(bs, i + 1, right, 2 * n + 2);
}

static void build_bst(struct ahnentafel_t* bs)
{
  bs->bst_size = 1;
  for (int i = 1; i < 32; i++)
  {
    if (bs->size < (1 << i))
    {
      bs->bst_size = (1 << i) - 1;
      break;
    }
  }
  bs->bst = (uint32_t *)malloc(sizeof(item_t) * bs->bst_size);
  // TODO: find sentinel value instead of 0
  memset(bs->bst, 0, sizeof(item_t) * bs->bst_size);
  fill_bst(bs, 0, bs->size - 1, 0);
}

void ahnentafel_fill(struct ahnentafel_t* bs, iter_t iter, void* opaque)
{
  for (int i = 0; i < bs->size; i++)
    bs->array[i] = *(iter(opaque));
  build_bst(bs);
}

void ahnentafel_generate_c(const struct ahnentafel_t* bs, FILE* stream)
{
  fputs(
  GENERATED_FILE_PROLOGUE
  "#define item_t " QUOTEME(item_t) "\n"
  "#define ARRLEN(a) (sizeof(a)/sizeof(*a))\n"
  "#define N 0\n"
  "static const item_t array[] =" S_EOL "{\n", stream);
  CALL_PRINT_ARRAY(item_t, stream, bs->bst, bs->bst_size);
  fputs(S_EOL "};\n"
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
    "int i = 0;" S_EOL
    "if (item == N)" S_EOL
      "return 0;" S_EOL
    "do" S_EOL
    "{" S_EOL
      "if (array[i] == item)" S_EOL
        "return &array[i];" S_EOL
      "if (array[i] > item)" S_EOL
        "i = 2 * i + 1;" S_EOL
      "else" S_EOL
        "i = 2 * i + 2;" S_EOL
    "}" S_EOL
    "while (i < (int)ARRLEN(array));" S_EOL
    "return 0;" S_EOL
  "}" S_EOL, stream);
}
