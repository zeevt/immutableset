#ifndef MAPPED_FILE_H_
#define MAPPED_FILE_H_

#ifdef WIN32
#include "stdint.h"
#else
#include <stdint.h>
#endif

struct mapped_file
{
  int64_t length;
  void*   data;
};

int munmap_file(struct mapped_file *mf);
struct mapped_file * mmap_file(const char* path);

#endif /* MAPPED_FILE_H_ */
