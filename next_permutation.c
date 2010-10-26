/*
This is a port from C++ to C of a small part of the C++ STL.
SGI's STL code comes with the following license:

 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996,1997
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
*/

static void iter_swap(unsigned char *a, unsigned char *b)
{
  unsigned char c = *a;
  *a = *b;
  *b = c;
}

static void reverse(unsigned char *first, unsigned char *last)
{
  --last;
  while (first < last) iter_swap(first++, last--);
}

int next_permutation(unsigned char *first, unsigned char *last)
{
  if (first == last) return 0;
  unsigned char *i = first;
  ++i;
  if (i == last) return 0;
  i = last;
  --i;
  for (;;)
  {
    unsigned char *ii = i--;
    if (*i < *ii)
    {
      unsigned char *j = last;
      while (!(*i < *--j));
      iter_swap(i, j);
      reverse(ii, last);
      return 1;
    }
    if (i == first)
    {
      reverse(first, last);
      return 0;
    }
  }
}

#ifdef TEST

#include <stdio.h>

int main()
{
  unsigned char test[] = {1,2,6,8};
  do
  {
    for (int i = 0; i < sizeof(test); i++) printf("%d ", test[i]);
    putchar('\n');
  }
  while (next_permutation(test, test + sizeof(test)));
  return 0;
}

#endif /* TEST */
