#ifndef READONLY_SET_H_
#define READONLY_SET_H_

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

#include "readonly_set_cfg.h"

typedef item_t* (*iter_t)(void* opaque);

struct readonly_set;

typedef struct readonly_set* (*allocate_fp)(int size);
typedef void (*cleanup_fp)(struct readonly_set* a_read_only_set);
typedef void (*fill_fp)(struct readonly_set* a_read_only_set, iter_t iter, void* opaque);
typedef void (*generate_c_fp)(const struct readonly_set* a_read_only_set, FILE* stream);

struct readonly_set_ops
{
  allocate_fp allocate;
  cleanup_fp cleanup;
  fill_fp fill;
  generate_c_fp generate_c;
};

#endif /* READONLY_SET_H_ */
