#ifndef WINNT_UTILS_H_
#define WINNT_UTILS_H_

#include <windows.h>
#include <strsafe.h>
#pragma warning(disable: 4505) /* unreferenced local function has been removed */

#define strcasecmp _stricmp

void Error(LPTSTR lpszFunction);

BSTR getBSTR(const char *ansistr);

HINSTANCE winnt_dlopen(const char* path);

#define dlopen(n,m) winnt_dlopen(n)
#define dlsym(h,n)  GetProcAddress(h,n)
#define dlclose(h)  (!FreeLibrary(h))
#define dlerror()   (0)

#define timestamp_t             LARGE_INTEGER
#define init_timestamper        winnt_init_timestamper
#define get_timestamp(t)        QueryPerformanceCounter(&t)
#define timestamp_diff(t1,t2)   winnt_timestamp_diff(&t1, &t2)
#define PRINTF_TIMESTAMP_STR    "%d.%09d"
#define PRINTF_TIMESTAMP_VAL(t) t.HighPart, t.LowPart

int winnt_init_timestamper(void);
void winnt_timestamp_diff(LARGE_INTEGER *t1, const LARGE_INTEGER *t2);

#endif /* WINNT_UTILS_H_ */
