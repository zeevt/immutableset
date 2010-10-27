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

#include "linear.h"
#include "print_utils.h"

ABSTRACT_RS_STRUCT(linear_t)

void linear_generate_c(const struct linear_t* bs, FILE* stream)
{
  fputs(
  GENERATED_FILE_PROLOGUE
  "#define item_t " QUOTEME(item_t) "\n"
  "static const item_t array[] =" S_EOL "{\n", stream);
  CALL_PRINT_ARRAY(item_t, stream, bs->array, bs->size);
  fputs(S_EOL "};\n"
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
#if 0
    "for (unsigned i = 0; i < sizeof(array)/sizeof(*array); i++)" S_EOL
      "if (array[i] == item)" S_EOL
        "return &array[i];" S_EOL
#else
#define CHECK(off) \
      "if (array[i+"#off"] == item)" S_EOL \
        "return &array[i+"#off"];" S_EOL

    "unsigned i = 0;"
    "for (; i < ((sizeof(array)/sizeof(*array)) & ~7); i+=8) {" S_EOL
        CHECK(0)
        CHECK(1)
        CHECK(2)
        CHECK(3)
        CHECK(4)
        CHECK(5)
        CHECK(6)
        CHECK(7)
    "}" S_EOL

    "for (; i < sizeof(array)/sizeof(*array); i++)" S_EOL
      "if (array[i] == item)" S_EOL
        "return &array[i];" S_EOL
#endif
    "return 0;" S_EOL
  "}" S_EOL, stream);
}
