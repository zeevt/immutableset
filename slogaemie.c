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

static int slogaemie_internal(unsigned char* output, int size, int sum, int min_num)
{
  if (sum < min_num) return 0;
  if (size == 1)
  {
    if (output[0] == sum) return 0;
    output[0] = (unsigned char)sum;
    return 1;
  }
  if (output[0] && slogaemie_internal(output + 1, size - 1, sum - output[0], output[0]))
    return 1;
  if (output[0]++ < min_num) output[0] = (unsigned char)min_num;
  for (int i = 1; i < size; i++) output[i] = 0;
  int remain = sum - output[0];
  if (remain < output[0]) return 0;
  return slogaemie_internal(output + 1, size - 1, remain, output[0]);
}

int slogaemie(unsigned char* output, int size, int sum)
{
  if (size <= 0) return 0;
  return slogaemie_internal(output, size, sum, output[0] ? output[0] : 1);
}

#ifdef TEST

#include <stdlib.h>
#include <stdio.h>

int main()
{
  for (int size = 1; size <= 9; size++)
  {
    unsigned char* test = calloc(size, 1);
    while (slogaemie(test, size, 9))
    {
      for (int i = 0; i < size; i++) printf("%d ", test[i]);
      putchar('\n');
    }
    free(test);
  }
  return 0;
}

#endif /* TEST */
