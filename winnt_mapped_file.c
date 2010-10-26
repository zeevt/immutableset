#include "winnt_utils.h"
#include "mapped_file.h"

struct winnt_mapped_file
{
  int64_t length;
  void*   data;
/* above fields are "inherited"! */
  HANDLE hFile;
  HANDLE hFileMapping;
};

int munmap_file(struct mapped_file *amf)
{
  struct winnt_mapped_file* mf = (struct winnt_mapped_file*)amf;
  if (mf->data && (0 == UnmapViewOfFile(mf->data)))
  {
    Error(TEXT("UnmapViewOfFile"));
    goto error;
  }
  mf->data = 0;
  if (mf->hFileMapping)
    CloseHandle(mf->hFileMapping);
  mf->hFileMapping = 0;
  if (mf->hFile)
    CloseHandle(mf->hFile);
  mf->hFile = 0;
  free(mf);
  return 0;
error:
  return 1;
}

struct mapped_file * mmap_file(const char* path)
{
  struct winnt_mapped_file *mf = (struct winnt_mapped_file *)malloc(sizeof(struct winnt_mapped_file));
  mf->hFile = 0;
  mf->hFileMapping = 0;
  mf->data = 0;

  BSTR unicodestr = getBSTR(path);
  if (!unicodestr)
    goto error;

  mf->hFile = CreateFile(
    unicodestr,
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
    NULL);

  SysFreeString(unicodestr);

  if (INVALID_HANDLE_VALUE == mf->hFile)
  {
    Error(TEXT("CreateFile"));
    goto error;
  }

  if (0 == GetFileSizeEx(mf->hFile, (PLARGE_INTEGER)&(mf->length)))
  {
    Error(TEXT("GetFileSizeEx"));
    goto error;
  }

  mf->hFileMapping = CreateFileMapping(
    mf->hFile,
    NULL,
    PAGE_READONLY,
    0,
    0,
    NULL);

  if (!mf->hFileMapping)
  {
    Error(TEXT("CreateFileMapping"));
    goto error;
  }

  mf->data = MapViewOfFile(mf->hFileMapping, FILE_MAP_READ, 0, 0, 0);

  if (!mf->data)
  {
    Error(TEXT("MapViewOfFile"));
    goto error;
  }

  return (struct mapped_file*)mf;
error:
  munmap_file((struct mapped_file*)mf);
  return NULL;
}
