// include/xsw-paths.h
/*
	Installation directories compiled into the programs.

	They can be given from the Makefile, for example
	-DXSW_DATA_DIR=\"/usr/local/share/games/xshipwars\" (the
	macOS Makefiles do this from PREFIX, since /usr/share and /home
	cannot be used there).
 */

#ifndef XSW_PATHS_H
#define XSW_PATHS_H

/* Client data (etc, images, sounds), used by client, monitor and unvedit. */
#ifndef XSW_DATA_DIR
# define XSW_DATA_DIR		"/usr/share/games/xshipwars"
#endif

/* Server toplevel directory. */
#ifndef SWSERV_DIR
# define SWSERV_DIR		"/home/swserv"
#endif

#endif	/* XSW_PATHS_H */
