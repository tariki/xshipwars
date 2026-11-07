// compat/win32/regex.h
/* MinGW has no POSIX regex. See compat.cpp. */
#ifndef XSW_WIN32_REGEX_H
#define XSW_WIN32_REGEX_H

#include <stddef.h>

#define REG_EXTENDED	1
#define REG_ICASE	2
#define REG_NOSUB	4

#define REG_NOMATCH	1
#define REG_BADPAT	2

typedef struct {
	void *re;
} regex_t;

typedef struct {
	int rm_so, rm_eo;
} regmatch_t;

extern int regcomp(regex_t *preg, const char *pattern, int cflags);
extern int regexec(
	const regex_t *preg, const char *string,
	size_t nmatch, regmatch_t *pmatch, int eflags
);
extern void regfree(regex_t *preg);

#endif	/* XSW_WIN32_REGEX_H */
