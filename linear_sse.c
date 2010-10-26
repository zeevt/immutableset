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

#include "linear_sse.h"
#include "print_utils.h"

ABSTRACT_RS_STRUCT(linear_sse_t)

void linear_sse_generate_c(const struct linear_sse_t* bs, FILE* stream)
{
  fputs(
  GENERATED_FILE_PROLOGUE
  "#include <emmintrin.h>\n"
  , stream);
  fprintf(stream, "#define CACHE_LINE_BYTES %d\n", CACHE_LINE_BYTES);
  fputs(
  "#define item_t " QUOTEME(item_t) "\n"
  "#ifdef WIN32\n"
  "#define ALIGN(n) __declspec(align(n))\n"
  "#else\n"
  "#define ALIGN(n) __attribute__ ((aligned(n)))\n"
  "#endif\n"
  "ALIGN(CACHE_LINE_BYTES) static const item_t array[] =" S_EOL "{\n"
  , stream);
  CALL_PRINT_ARRAY(item_t, stream, bs->array, bs->size);
  fputs(
  S_EOL "};\n"
  "#define ARRLEN(a) (sizeof(a)/sizeof(*a))\n"
  "#define VEC_SIZE  16\n"
  "#define VEC_ITEMS (VEC_SIZE/sizeof(item_t))\n"
  FUNCTION_DEFINITION S_EOL
  "{" S_EOL
#if item_bits == 64
    "__m128i s = _mm_set_epi32(item >> 32, item & UINT32_MAX, item >> 32, item & UINT32_MAX);" S_EOL
#else
    "__m128i s = _mm_set1_epi" QUOTEME(item_bits) "(item);" S_EOL
#endif
    "int n = ARRLEN(array);" S_EOL
    "n -= n % VEC_ITEMS;" S_EOL
    "const __m128i *p = (__m128i*)array;" S_EOL
    "for (; p < (__m128i*)&array[n]; p++)" S_EOL
    "{" S_EOL
      "__m128i g = *p;" S_EOL
#if item_bits == 16
      "__m128i temp = _mm_cmpeq_epi" QUOTEME(item_bits) "(s, g);" S_EOL
      "unsigned ans = _mm_movemask_epi8(temp);" S_EOL
      "if (ans)" S_EOL
        "for (unsigned i = 0; i < VEC_ITEMS; i++)" S_EOL
          "if (ans & (1 << (i*2)))" S_EOL
            "return (item_t*)p + i;" S_EOL
#elif item_bits == 32
      "__m128i temp = _mm_cmpeq_epi" QUOTEME(item_bits) "(s, g);" S_EOL
      "unsigned ans = _mm_movemask_ps(*(__m128*)&temp);" S_EOL
      "if (ans)" S_EOL
        "for (unsigned i = 0; i < VEC_ITEMS; i++)" S_EOL
          "if (ans & (1 << i))" S_EOL
            "return (item_t*)p + i;" S_EOL
#elif item_bits == 64
      "__m128i temp = _mm_cmpeq_epi32(s, g);" S_EOL
      "unsigned ans = _mm_movemask_ps(*(__m128*)&temp);" S_EOL
      "if ((ans & 3) == 3) return (item_t*)p;" S_EOL
      "if ((ans & 12) == 12) return (item_t*)p + 1;" S_EOL
#else
#error item_bits must be wither 16,32 or 64 !
#endif
    "}" S_EOL
    "for (item_t *pp = (item_t*)p; pp <= &array[ARRLEN(array) - 1]; pp++)" S_EOL
      "if (*pp == item)" S_EOL
        "return pp;" S_EOL
    "return 0;" S_EOL
  "}" S_EOL, stream);
}
