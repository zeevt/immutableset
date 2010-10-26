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

#include "unroll_bsearch.h"
#include "print_utils.h"

ABSTRACT_RS_STRUCT(unroll_bsearch_t)

static void print_items(FILE* stream, const struct unroll_bsearch_t* rs)
{
  for (int i = 0; i < rs->size; i++)
    fprintf(stream, "static const item_t n%d = %" PR_item_t ";\n", i, rs->array[i]);
}

static void print_if(FILE* stream, const item_t *array, int l, int r)
{
  int i = l + (r - l) / 2;
  fprintf(stream, "if (item == n%d) return &n%d;\n", i, i);
  if ((i != l) || (i != r))
    fprintf(stream, "if (item %c n%d)\n", (i != l) ? '<' : '>', i);
  if (i != l)
  {
    fputs("{\n", stream);
    print_if(stream, array, l, i - 1);
    fputs("}\n", stream);
  }
  if (i != r)
  {
    if (i != l)
      fputs("else\n", stream);
    fputs("{\n", stream);
    print_if(stream, array, i + 1, r);
    fputs("}\n", stream);
  }
}

void unroll_bsearch_generate_c(const struct unroll_bsearch_t* rs, FILE* stream)
{
  fputs(
  GENERATED_FILE_PROLOGUE
  "#define item_t " QUOTEME(item_t) "\n"
  , stream);
  print_items(stream, rs);
  fputs(
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
  , stream);
  print_if(stream, rs->array, 0, rs->size - 1);
  fputs(
    "return 0;" S_EOL
  "}" S_EOL
  , stream);
}
