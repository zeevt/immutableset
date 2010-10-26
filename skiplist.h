#ifndef SKIPLIST_H_
#define SKIPLIST_H_

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

#include "readonly_set.h"

struct skiplist_t;

struct skiplist_t* skiplist_allocate(int size);

void skiplist_cleanup(struct skiplist_t* sl);

void skiplist_fill(struct skiplist_t* sl, iter_t iter, void* opaque);

void skiplist_generate_c(const struct skiplist_t* sl, FILE* stream);

static const struct readonly_set_ops skiplist =
{
  (allocate_fp)&skiplist_allocate,
  (cleanup_fp)&skiplist_cleanup,
  (fill_fp)&skiplist_fill,
  (generate_c_fp)&skiplist_generate_c
};

#endif /* SKIPLIST_H_ */
