#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "mapped_file.h"

struct unix_mapped_file
{
  int64_t length;
  void*   data;
/* above fields are "inherited"! */
  int fd;
};

int munmap_file(struct mapped_file *amf)
{
  struct unix_mapped_file* mf = (struct unix_mapped_file*)amf;
  if (mf->data)
  {
    munmap(mf->data, mf->length);
    mf->data = NULL;
  }
  if (mf->fd != -1)
  {
    close(mf->fd);
    mf->fd = -1;
  }
  free(mf);
  return 0;
}

struct mapped_file* mmap_file(const char* path)
{
  struct unix_mapped_file *mf = (struct unix_mapped_file *)malloc(sizeof(struct unix_mapped_file));
  struct stat st_buf;
  mf->data = NULL;
  mf->fd = -1;
  if (-1 == (mf->fd = open(path, O_RDONLY)))
  {
    fprintf(stderr, "Could not open %s!\n", path);
    goto error;
  }
  if (fstat(mf->fd, &st_buf))
    goto error;
  mf->length = st_buf.st_size;
  if (MAP_FAILED == (mf->data = mmap(NULL, mf->length, PROT_READ, MAP_SHARED, mf->fd, 0)))
  {
    fprintf(stderr, "Could not mmap %s!\n", path);
    goto error;
  }
  return (struct mapped_file*)mf;
error:
  munmap_file((struct mapped_file*)mf);
  return NULL;
}
