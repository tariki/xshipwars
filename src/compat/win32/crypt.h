// compat/win32/crypt.h
/*
	crypt() is declared by compat.h. This file hides the <crypt.h> of
	zlib's minizip in the MinGW include directory, which has nothing to
	do with it and which server/crypt.cpp would otherwise pick up.
 */
