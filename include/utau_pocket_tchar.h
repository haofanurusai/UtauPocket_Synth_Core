#ifndef UTAU_POCKET_TCHAR_H
#define UTAU_POCKET_TCHAR_H
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#if !defined(_UNICODE) || !defined(UNICODE)
#error UtauPocket Windows ABI requires UNICODE and _UNICODE
#endif
#include <tchar.h>
#else
#include <strings.h>
typedef char TCHAR;
#define _T(s) s
#define _tfopen fopen
#define _tremove remove
#define _tcslen strlen
#define _tcscpy strcpy
#define _tcscat strcat
#define _tcsrchr strrchr
#define _tcsncpy strncpy
#define _tcsicmp strcasecmp
#define _sntprintf snprintf
#define _ftprintf fprintf
#endif
#endif
