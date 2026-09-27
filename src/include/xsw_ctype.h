// xsw_ctype.h
// This is intended as a prototype for files using the global/ctype.cpp file.

/* isblank() is standard since C99/C++11 and comes from <ctype.h>.
 * The old `bool isblank(int)' prototype here conflicted with it and was
 * never defined anywhere; global/ctype.cpp only provides these helpers.
 */
extern bool isblankChar(char c);
extern bool isblankInt(int c);


extern void ctype_dummy_func();
