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

#include "bbst.h"
#include "print_utils.h"

ABSTRACT_RS_STRUCT(bbst_t)

static int print_nodes(FILE* stream, const item_t *array, int l, int r, int n)
{
  int i = l + (r - l) / 2;
  int l_n = 0, r_n = 0;
  if (i != l)
    l_n = print_nodes(stream, array, l, i - 1, n);
  if (i != r)
    r_n = print_nodes(stream, array, i + 1, r, (l_n ? l_n : n));
  int new_n = (r_n ? r_n : (l_n ? l_n : n)) + 1;
  fprintf(stream, "static const node n%d = { %" PR_item_t ", ", new_n, array[i]);
  if (l_n)
    fprintf(stream, "&n%d", l_n);
  else
    fputs("0", stream);
  fputs(", ", stream);
  if (r_n)
    fprintf(stream, "&n%d", r_n);
  else
    fputs("0", stream);
  fputs(" };" S_EOL, stream);
  return new_n;
}

void bbst_generate_c(const struct bbst_t* bs, FILE* stream)
{
  fputs(
  GENERATED_FILE_PROLOGUE
  "#define item_t " QUOTEME(item_t) "\n"
  "typedef struct node" S_EOL
  "{" S_EOL
    "item_t item;" S_EOL
    "const struct node *left;" S_EOL
    "const struct node *right;" S_EOL
  "} node;" S_EOL, stream);
  int root = print_nodes(stream, bs->array, 0, bs->size - 1, 0);
  fprintf(stream,
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
    "const node *p = &n%d;" S_EOL
    "do" S_EOL
    "{" S_EOL
      "if (p->item == item)" S_EOL
        "return &(p->item);" S_EOL
      "if (p->item > item)" S_EOL
        "p = p->left;" S_EOL
      "else" S_EOL
        "p = p->right;" S_EOL
    "}" S_EOL
    "while (p);" S_EOL
    "return 0;" S_EOL
  "}" S_EOL, root);
}
