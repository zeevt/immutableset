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

#include "my_bsearch.h"
#include "print_utils.h"

ABSTRACT_RS_STRUCT(my_bsearch_t)

void my_bsearch_generate_c(const struct my_bsearch_t* bs, FILE* stream)
{
  fputs(
  GENERATED_FILE_PROLOGUE
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
  "}" S_EOL
  "static const item_t* my_bsearch(item_t v, int size, const item_t* array)" S_EOL
  "{" S_EOL
    "int left = 0, right = size - 1;" S_EOL
    "for (;;)" S_EOL
    "{" S_EOL
      "int middle = left + ((unsigned)(right - left) / 2);" S_EOL
      "int res = cmp_item(&array[middle], &v);" S_EOL
      "if (res > 0)" S_EOL
        "right = middle - 1; " S_EOL
      "else if (res < 0) " S_EOL
        "left = middle + 1;" S_EOL
      "else " S_EOL
        "return &array[middle];" S_EOL
      "if (left > right)" S_EOL
        "return 0;" S_EOL
    "}" S_EOL
  "}\n"
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
    "return my_bsearch(item, sizeof(array)/sizeof(array[0]), array);" S_EOL
  "}" S_EOL, stream);
}
