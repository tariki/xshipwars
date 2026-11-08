// compat/win32/compat.h
/*
	POSIX compatibility layer for the native Windows build (MinGW-w64).

	Makefile.Windows force-includes this file (-include) in front of
	every source file, and puts this directory on the include path so
	that <sys/socket.h> and the like resolve to the empty headers here.
	The callers stay as they are on Linux.

	Everything that needs code is in compat.cpp.
 */

#ifndef XSW_WIN32_COMPAT_H
#define XSW_WIN32_COMPAT_H

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#define NOGDI		/* Rectangle(), etc. */
#define NOUSER		/* CreateWindow(), etc. */
#define NOMINMAX

/* off_t is 32 bits otherwise, too small for the memory sizes in mf.cpp. */
#define _FILE_OFFSET_BITS	64

/* All the headers that the macros below would otherwise break. */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>


/* Types. It is 64 bits wide in MinGW, and printed with %i. */
#define pid_t		int

typedef int uid_t;
typedef int gid_t;

/* Checked by os.h, which would otherwise redefine the fixed width types. */
#define __BIT_TYPES_DEFINED__
typedef uint8_t		u_int8_t;
typedef uint16_t	u_int16_t;
typedef uint32_t	u_int32_t;
typedef uint64_t	u_int64_t;


/* Paths. os.h includes <paths.h> if this is not defined. */
#define _PATH_VI	"vi"

#ifndef NAME_MAX
# define NAME_MAX	255
#endif

/*
 *	260 here. The same size as on Linux, so that values from the
 *	configuration files (up to CFG_VALUE_MAX) fit in the path buffers
 *	instead of being cut. Makefile.inc gives the stack room for them.
 */
#undef PATH_MAX
#define PATH_MAX	4096

/* The default is the root of the current drive. */
#undef P_tmpdir
#define P_tmpdir	"."


extern "C" size_t strlcpy(char *dest, const char *src, size_t size);
#define bzero(p, n)	memset((p), 0, (n))


/* Files. There are no symbolic links, sockets or modes in the C library. */
#ifndef S_ISLNK
# define S_ISLNK(m)	0
#endif
#ifndef S_ISSOCK
# define S_ISSOCK(m)	0
#endif

#define lstat		stat
#define mkdir(p, m)	_mkdir(p)

extern int xsw_readlink(const char *path, char *buf, size_t size);
#define readlink	xsw_readlink

/* Returns the path with / as the separator, which the path functions use. */
extern char *xsw_getcwd(char *buf, int size);
#define getcwd		xsw_getcwd


/* Sockets. */
#define F_SETFL		4
#define O_NONBLOCK	0x4000

/* Only used as fcntl(s, F_SETFL, O_NONBLOCK). */
extern int xsw_fcntl(int s, int cmd, int arg);
#define fcntl		xsw_fcntl

/* close() is called on both files and sockets. */
extern int xsw_close(int fd);
#define close		xsw_close

/* These set errno from WSAGetLastError(), which the callers look at. */
extern int xsw_socket(int domain, int type, int protocol);
extern int xsw_accept(int s, struct sockaddr *addr, socklen_t *len);
extern int xsw_connect(int s, const struct sockaddr *addr, socklen_t len);
extern int xsw_recv(int s, void *buf, size_t len, int flags);
extern int xsw_send(int s, const void *buf, size_t len, int flags);
extern int xsw_select(
	int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t
);
#define socket(d, t, p)		xsw_socket((d), (t), (p))
#define accept(s, a, l)		xsw_accept((s), (a), (l))
#define connect(s, a, l)	xsw_connect((s), (a), (l))
#define recv(s, b, l, f)	xsw_recv((s), (b), (l), (f))
#define send(s, b, l, f)	xsw_send((s), (b), (l), (f))
#define select(n, r, w, e, t)	xsw_select((n), (r), (w), (e), (t))


/* Signals that the C library does not have. They are never delivered. */
#define SIGHUP		1
#define SIGQUIT		3
#define SIGTRAP		5
#define SIGBUS		7
#define SIGKILL		9
#define SIGUSR1		10
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		23
#define SIGTTOU		24
#define SIGURG		25
#define SIGXCPU		26
#define SIGXFSZ		27
#define SIGVTALRM	28
#define SIGPROF		29
#define SIGWINCH	30
#define SIGIO		31

/* The C library aborts on a signal number that it does not know. */
typedef void (*xsw_sighandler_t)(int);
extern xsw_sighandler_t xsw_signal(int s, xsw_sighandler_t handler);
#define signal		xsw_signal

/*
 *	kill(pid, SIGTERM) signals the event of a process that called
 *	Win32WatchTerm(), which raises SIGTERM from Win32PollTerm() in its
 *	main loop, so that it can save before exiting as it does on UNIX.
 *	Without that event, the process is terminated without notice.
 *	Win32WatchTerm() has to come before signal(SIGINT, ...). Ctrl+C and
 *	closing the console then come through the same event, so SIGTERM
 *	is what the process sees for them.
 */
extern int xsw_kill(pid_t pid, int s);
#define kill		xsw_kill
extern void Win32WatchTerm(void);
extern void Win32PollTerm(void);


/* Child processes. There is no fork(); see prochandle.cpp. */
struct rusage;
#define WNOHANG		1
extern pid_t wait3(int *status, int options, struct rusage *rusage);

/*
 *	Runs the command line, with stdout and stderr going to the given
 *	files if not NULL. Returns the process id, or -1 on error. If
 *	block is true, then it returns when the process has exited.
 */
extern pid_t Win32Spawn(
	const char *cmd, FILE *stdout_fp, FILE *stderr_fp, int block
);


/* The DES crypt(3), for the passwords stored in the universe files. */
extern char *crypt(const char *key, const char *salt);


/*
 *	The GUI programs have no console when started from the Explorer,
 *	so compat.cpp wraps main() to show why they could not start.
 */
#ifdef XSW_WIN32_GUI
# define main		xsw_main
#endif

#endif	/* XSW_WIN32_COMPAT_H */
