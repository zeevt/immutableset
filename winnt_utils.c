#include <windows.h>
#include "winnt_utils.h"

void Error(LPTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

BSTR getBSTR(const char *ansistr)
{
  // http://www.codeguru.com/forum/showthread.php?t=231165
  int lenA = lstrlenA(ansistr);
  BSTR unicodestr = 0;

  int lenW = MultiByteToWideChar(CP_ACP, 0, ansistr, lenA, 0, 0);
  if (lenW > 0)
  {
    // Check whether conversion was successful
    unicodestr = SysAllocStringLen(0, lenW);
    MultiByteToWideChar(CP_ACP, 0, ansistr, lenA, unicodestr, lenW);
  }
  else
  {
    Error(TEXT("MultiByteToWideChar"));
    return 0;
  }

  return unicodestr;
}

HINSTANCE winnt_dlopen(const char* path)
{
  HINSTANCE result;
  BSTR unicodestr = getBSTR(path);
  result = LoadLibrary(unicodestr);
  SysFreeString(unicodestr);
  return result;
}

static LARGE_INTEGER freq;

int winnt_init_timestamper(void)
{
  BOOL res = QueryPerformanceFrequency(&freq);
  return !res;
}

void winnt_timestamp_diff(LARGE_INTEGER *t1, const LARGE_INTEGER *t2)
{
  LARGE_INTEGER temp;
  temp.QuadPart = t2->QuadPart - t1->QuadPart;
  LARGE_INTEGER res;
  res.HighPart = temp.QuadPart / freq.QuadPart;
  res.LowPart = ((temp.QuadPart % freq.QuadPart) * 1000000000) / freq.QuadPart;
  *t1 = res;
}
