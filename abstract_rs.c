
#include <stdlib.h>
#include <stdio.h>
#include "abstract_rs.h"

ABSTRACT_RS_STRUCT(abstract_rs_t)

struct abstract_rs_t* abstract_rs_allocate(int size)
{
  struct abstract_rs_t* bs = (struct abstract_rs_t*)malloc(sizeof(int) + sizeof(item_t) * size);
  bs->size = size;
  return bs;
}

void abstract_rs_cleanup(struct abstract_rs_t* bs)
{
  if (!bs) return;
  free(bs);
}

void abstract_rs_fill(struct abstract_rs_t* bs, iter_t iter, void* opaque)
{
  for (int i = 0; i < bs->size; i++)
    bs->array[i] = *(iter(opaque));
}
