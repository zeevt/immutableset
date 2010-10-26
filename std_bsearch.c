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

#include "std_bsearch.h"
#include "print_utils.h"

ABSTRACT_RS_STRUCT(std_bsearch_t)

void std_bsearch_generate_c(const struct std_bsearch_t* bs, FILE* stream)
{
  fputs(
  GENERATED_FILE_PROLOGUE
  "#include <stdlib.h>\n"
  "#define item_t " QUOTEME(item_t) "\n"
  "static const item_t array[] =" S_EOL "{\n", stream);
  CALL_PRINT_ARRAY(item_t, stream, bs->array, bs->size);
  fputs(S_EOL "};\n"
  "static int cmp_item(const void *a, const void *b)" S_EOL
  "{" S_EOL
    "item_t x = *(item_t*)a;" S_EOL
    "item_t y = *(item_t*)b;" S_EOL
    "if (x < y)" S_EOL
      "return -1;" S_EOL
    "else if (x > y)" S_EOL
      "return 1;" S_EOL
    "else " S_EOL
      "return 0;" S_EOL
  "}\n"
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
    "return (const item_t*)bsearch(&item, array, sizeof(array)/sizeof(*array), sizeof(item_t), &cmp_item);" S_EOL
  "}" S_EOL, stream);
}
