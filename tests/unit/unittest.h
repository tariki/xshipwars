// tests/unit/unittest.h
/*
	A minimal unit test helper, see scripts/unittest.sh.

	Each check prints a line only when it fails. UNITTEST_END()
	prints the count and gives the exit status of the test.
 */

#ifndef UNITTEST_H
#define UNITTEST_H

#include <stdio.h>
#include <string.h>

static int unittest_total = 0, unittest_fails = 0;

static inline void unittest_check_str(
	const char *what, const char *got, const char *want,
	const char *file, int line
)
{
	unittest_total++;
	if(got == NULL)
	    got = "(null)";
	if(strcmp(got, want))
	{
	    unittest_fails++;
	    printf("FAIL  %s:%d: %s\n      got  \"%s\"\n      want \"%s\"\n",
		file, line, what, got, want
	    );
	}
}

static inline void unittest_check_int(
	const char *what, long got, long want, const char *file, int line
)
{
	unittest_total++;
	if(got != want)
	{
	    unittest_fails++;
	    printf("FAIL  %s:%d: %s\n      got  %ld\n      want %ld\n",
		file, line, what, got, want
	    );
	}
}

/* The call is printed as written, so that a failure says what it was. */
#define CHECK_STR(call, want) \
	unittest_check_str(#call, (call), (want), __FILE__, __LINE__)
#define CHECK_INT(call, want) \
	unittest_check_int(#call, (long)(call), (long)(want), __FILE__, __LINE__)

#define UNITTEST_END() \
	(printf("%d of %d passed\n", unittest_total - unittest_fails, \
		unittest_total), \
	 (unittest_fails > 0) ? 1 : 0)

#endif	/* UNITTEST_H */
