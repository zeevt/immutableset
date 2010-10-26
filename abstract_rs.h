#ifndef ABSTRACT_RS_H_
#define ABSTRACT_RS_H_

#include "readonly_set.h"

#define ABSTRACT_RS_STRUCT(name) \
struct name \
{ \
  int size; \
  item_t array[C99_FLEXIBLE_ARRAY_MEMBER_LENGTH]; \
};

struct abstract_rs_t;

struct abstract_rs_t* abstract_rs_allocate(int size);

void abstract_rs_cleanup(struct abstract_rs_t* bs);

void abstract_rs_fill(struct abstract_rs_t* bs, iter_t iter, void* opaque);

#endif /* ABSTRACT_RS_H_ */
