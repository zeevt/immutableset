#ifndef GHASH_H_
#define GHASH_H_

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
#include "abstract_rs.h"

struct ghash_t;

struct ghash_t* ghash_allocate(int size);

void ghash_fill(struct ghash_t* bs, iter_t iter, void* opaque);

void ghash_generate_c(const struct ghash_t* bs, FILE* stream);

static const struct readonly_set_ops ghash =
{
  (allocate_fp)&ghash_allocate,
  (cleanup_fp)&abstract_rs_cleanup,
  (fill_fp)&ghash_fill,
  (generate_c_fp)&ghash_generate_c
};

#endif /* GHASH_H_ */
