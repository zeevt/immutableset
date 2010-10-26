#ifndef BIGBUCKET_H_
#define BIGBUCKET_H_

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

struct bigbucket_t;

struct bigbucket_t* bigbucket_allocate(int size);

void bigbucket_cleanup(struct bigbucket_t* bs);

void bigbucket_fill(struct bigbucket_t* bs, iter_t iter, void* opaque);

void bigbucket_generate_c(const struct bigbucket_t* bs, FILE* stream);

static const struct readonly_set_ops bigbucket =
{
  (allocate_fp)&bigbucket_allocate,
  (cleanup_fp)&bigbucket_cleanup,
  (fill_fp)&bigbucket_fill,
  (generate_c_fp)&bigbucket_generate_c
};

#endif /* BIGBUCKET_H_ */
