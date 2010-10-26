#ifndef PRINT_UTILS_H_
#define PRINT_UTILS_H_

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

#ifndef UNUSED
#ifdef WIN32
#define UNUSED 
#pragma warning(disable: 4505) /* unreferenced local function has been removed */
#else
#define UNUSED __attribute__ ((unused))
#endif
#endif

extern const char* hex_chars;

#define min(a,b) (((a)>(b))?(b):(a))

#define LINE_WIDTH 80

#define S_EOL "\n"

#define DEFINE_PRINT_FUNCS(type_name, ...) \
  static inline void to_hex_##type_name(int size, type_name item, char* buf) \
  { \
    for (int i = 0; i < size; i++) \
    { \
      buf[(size - i - 1) * 2] = hex_chars[(item & 0xf0) >> 4]; \
      buf[(size - i - 1) * 2 + 1] = hex_chars[item & 0x0f]; \
      item >>= 8; \
    } \
  } \
  \
  __VA_ARGS__ void print_##type_name##_item(type_name item, FILE *stream) \
  { \
    char buf[2 + sizeof(type_name) * 2]; \
    buf[0] = '0'; \
    buf[1] = 'x'; \
    to_hex_##type_name(sizeof(type_name), item, buf+2); \
    fwrite(buf, 1, sizeof(buf), stream); \
  } \
  \
  __VA_ARGS__ void print_##type_name##_array(int size, FILE* stream, const type_name *array, int length) \
  { \
    const int item_length = 2 + size * 2 + 1; \
    const int items_in_line = LINE_WIDTH / item_length; \
    char buf[LINE_WIDTH + 1]; \
    int i = 0; \
    for ( ; i < items_in_line; i++) \
    { \
      buf[i * item_length + 0] = '0'; \
      buf[i * item_length + 1] = 'x'; \
      buf[i * item_length + size * 2 + 2] = ','; \
    } \
    buf[i * item_length] = '\n'; \
    for (int line = 0; length; line++) \
    { \
      i = 0; \
      for ( ; i < min(items_in_line, length); i++) \
        to_hex_##type_name(size, array[line * items_in_line + i], &buf[i * item_length + 2]); \
      length -= i; \
      fwrite(buf, 1, item_length * i + !!length, stream); \
    } \
  }

#define DECLARE_PRINT_FUNCS(type_name) \
  void print_##type_name##_item(type_name item, FILE *stream); \
  void print_##type_name##_array(int size, FILE* stream, const type_name *array, int length); \

DECLARE_PRINT_FUNCS(item_t)

#define CALL_PRINT_ARRAY(type_name, stream, array, length) \
  print_##type_name##_array(sizeof(type_name), stream, array, length)

#define CALL_PRINT_ITEM(type_name, item, stream) \
  print_##type_name##_item(item, stream)

#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)

#define FUNCTION_DEFINITION \
  "#ifdef WIN32\n" \
  "#define EXPORTED __declspec(dllexport)\n" \
  "#else\n" \
  "#define EXPORTED __attribute__((visibility(\"default\")))\n" \
  "#endif\n" \
  "#ifdef __cplusplus\n" \
  "extern \"C\" {\n" \
  "#endif\n" \
  "EXPORTED const item_t* is_in_set(const item_t item);\n" \
  "#ifdef __cplusplus\n" \
  "}\n" \
  "#endif\n" \
  "EXPORTED const item_t* is_in_set(const item_t item)"

#define GENERATED_FILE_PROLOGUE \
  "#ifdef WIN32\n" \
  "#include \"stdint.h\"\n" \
  "#else\n" \
  "#include <stdint.h>\n" \
  "#endif\n"

#endif /* PRINT_UTILS_H_ */
