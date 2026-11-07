// compat/win32/fnmatch.h
#ifndef XSW_WIN32_FNMATCH_H
#define XSW_WIN32_FNMATCH_H

#define FNM_NOMATCH	1

extern "C" int fnmatch(const char *pattern, const char *string, int flags);

#endif	/* XSW_WIN32_FNMATCH_H */
