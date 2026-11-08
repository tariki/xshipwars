// compat/win32/compat.cpp
/*
	POSIX compatibility layer for the native Windows build. See
	compat.h.
 */

#include <regex>
#include <string>

#include <fnmatch.h>
#include <regex.h>
#include <ctype.h>
#include <mmsystem.h>

#ifdef XSW_WIN32_GUI
/*   MessageBoxA() of user32, which NOUSER in compat.h leaves out (and
 *   <winuser.h> cannot be included again). Not SDL's message box, which
 *   cannot show when SDL's video failed to start.
 */
extern "C" __declspec(dllimport) int WINAPI MessageBoxA(
	HWND hwnd, LPCSTR text, LPCSTR caption, UINT type
);
# define MB_OK		0x00000000
# define MB_ICONERROR	0x00000010
#endif

/* The real functions, in this file only. */
#undef main
#undef socket
#undef accept
#undef connect
#undef recv
#undef send
#undef select
#undef close
#undef signal
#undef kill
#undef getcwd
#undef fcntl
#undef readlink


/* Maps the last Winsock error to errno. */
static void set_errno_from_wsa(void)
{
	switch(WSAGetLastError())
	{
	  case WSAEWOULDBLOCK:	errno = EWOULDBLOCK;	break;
	  case WSAEINPROGRESS:	errno = EINPROGRESS;	break;
	  case WSAEINTR:	errno = EINTR;		break;
	  case WSAEINVAL:	errno = EINVAL;		break;
	  case WSAEBADF:
	  case WSAENOTSOCK:	errno = EBADF;		break;
	  case WSAENOTCONN:	errno = ENOTCONN;	break;
	  case WSAECONNRESET:	errno = ECONNRESET;	break;
	  case WSAECONNREFUSED:	errno = ECONNREFUSED;	break;
	  case WSAETIMEDOUT:	errno = ETIMEDOUT;	break;
	  /* What a write to a closed connection gives on UNIX. */
	  case WSAESHUTDOWN:
	  case WSAECONNABORTED:	errno = EPIPE;		break;
	  default:		errno = EIO;		break;
	}
}


/*
 *	Set up before main():
 *
 *	Winsock needs initializing, files are opened as binary as they are
 *	on UNIX (the programs read and write LF, and seek in the files),
 *	and HOME and LOGNAME come from their Windows counterparts.
 */
static void set_env_default(const char *name, const char *value)
{
	if((getenv(name) == NULL) && (value != NULL))
	{
	    std::string s(value);
	    for(size_t i = 0; i < s.size(); i++)
		if(s[i] == '\\')
		    s[i] = '/';
	    _putenv_s(name, s.c_str());
	}
}

#ifdef XSW_WIN32_GUI
/*
 *	The GUI programs have no console of their own. Their output goes
 *	to the console that they were started from, if any. Started from
 *	the Explorer there is none, so stderr is kept in a temporary file
 *	(the D mode deletes it with the process) for main() below to show.
 */
static bool stderr_captured = false;

static bool has_std_handle(DWORD n)
{
	HANDLE h = GetStdHandle(n);

	return(
	    (h != NULL) && (h != INVALID_HANDLE_VALUE) &&
	    (GetFileType(h) != FILE_TYPE_UNKNOWN)
	);
}

static void setup_gui_stdio(void)
{
	bool has_out = has_std_handle(STD_OUTPUT_HANDLE);
	bool has_err = has_std_handle(STD_ERROR_HANDLE);
	char dir[MAX_PATH], path[MAX_PATH];

	if(AttachConsole(ATTACH_PARENT_PROCESS))
	{
	    if(!has_out)
		freopen("CONOUT$", "w", stdout);
	    if(!has_err)
		freopen("CONOUT$", "w", stderr);
	}
	else if(!has_err &&
	    GetTempPathA(sizeof(dir), dir) &&
	    GetTempFileNameA(dir, "xsw", 0, path) &&
	    (freopen(path, "w+D", stderr) != NULL)
	)
	{
	    stderr_captured = true;
	}
}

/* The program's main(), see compat.h. */
extern int xsw_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	int status = xsw_main(argc, argv);

	if((status != 0) && stderr_captured)
	{
	    char mesg[2048];
	    size_t len;

	    fflush(stderr);
	    rewind(stderr);
	    len = fread(mesg, 1, sizeof(mesg) - 1, stderr);
	    mesg[len] = '\0';

	    MessageBoxA(
		NULL, (len > 0) ? mesg : "The program could not start.",
		"XShipWars", MB_OK | MB_ICONERROR
	    );
	}
	return(status);
}
#endif	/* XSW_WIN32_GUI */

namespace {
struct Win32Init
{
	Win32Init()
	{
	    WSADATA wsa;

	    WSAStartup(MAKEWORD(2, 2), &wsa);
	    timeBeginPeriod(1);		/* usleep() rounds up to 15.6ms. */
	    _fmode = _O_BINARY;
	    set_env_default("HOME", getenv("USERPROFILE"));
	    set_env_default("LOGNAME", getenv("USERNAME"));
#ifdef XSW_WIN32_GUI
	    setup_gui_stdio();
#endif
	}
} win32_init;
}


/* Strings. */
extern "C" size_t strlcpy(char *dest, const char *src, size_t size)
{
	size_t len = strlen(src);

	if(size > 0)
	{
	    size_t n = (len < size) ? len : size - 1;
	    memcpy(dest, src, n);
	    dest[n] = '\0';
	}
	return(len);
}


/* Files. */
int xsw_readlink(const char *path, char *buf, size_t size)
{
	errno = EINVAL;		/* Not a symbolic link. */
	return(-1);
}

char *xsw_getcwd(char *buf, int size)
{
	char *rtn = _getcwd(buf, size);

	if(rtn != NULL)
	    for(char *s = rtn; *s != '\0'; s++)
		if(*s == '\\')
		    *s = '/';
	return(rtn);
}


/* Sockets. */
int xsw_fcntl(int s, int cmd, int arg)
{
	u_long nonblock = (arg & O_NONBLOCK) ? 1 : 0;

	if(cmd != F_SETFL)
	{
	    errno = EINVAL;
	    return(-1);
	}
	if(ioctlsocket((SOCKET)s, FIONBIO, &nonblock) == SOCKET_ERROR)
	{
	    set_errno_from_wsa();
	    return(-1);
	}
	return(0);
}

int xsw_close(int fd)
{
	int type;
	int len = sizeof(type);

	/* Only a socket has this option. */
	if(!getsockopt(
	    (SOCKET)fd, SOL_SOCKET, SO_TYPE, (char *)&type, &len
	))
	    return(closesocket((SOCKET)fd));

	return(_close(fd));
}

/*
 *	A SOCKET is 64 bits wide, but its value is a handle, and handles
 *	are defined to fit in 32 bits. INVALID_SOCKET converts to -1.
 */
int xsw_socket(int domain, int type, int protocol)
{
	SOCKET s = socket(domain, type, protocol);

	if(s == INVALID_SOCKET)
	    set_errno_from_wsa();
	return((int)s);
}

int xsw_accept(int s, struct sockaddr *addr, socklen_t *len)
{
	SOCKET rtn = accept((SOCKET)s, addr, len);

	if(rtn == INVALID_SOCKET)
	    set_errno_from_wsa();
	return((int)rtn);
}

int xsw_connect(int s, const struct sockaddr *addr, socklen_t len)
{
	int rtn = connect((SOCKET)s, addr, len);

	if(rtn == SOCKET_ERROR)
	    set_errno_from_wsa();
	return(rtn);
}

int xsw_recv(int s, void *buf, size_t len, int flags)
{
	int rtn = recv((SOCKET)s, (char *)buf, (int)len, flags);

	if(rtn == SOCKET_ERROR)
	    set_errno_from_wsa();
	return(rtn);
}

int xsw_send(int s, const void *buf, size_t len, int flags)
{
	int rtn = send((SOCKET)s, (const char *)buf, (int)len, flags);

	if(rtn == SOCKET_ERROR)
	    set_errno_from_wsa();
	return(rtn);
}

int xsw_select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
	int rtn = select(n, r, w, e, t);

	if(rtn == SOCKET_ERROR)
	    set_errno_from_wsa();
	return(rtn);
}


/* Signals. */
static HANDLE term_event = NULL;

xsw_sighandler_t xsw_signal(int s, xsw_sighandler_t handler)
{
	switch(s)
	{
	  case SIGINT:
	    /*   The C library would run the handler in another thread,
	     *   at the same time as the main loop. console_ctrl() takes
	     *   care of Ctrl+C in a process that polls.
	     */
	    if(term_event != NULL)
		return(SIG_DFL);
	    return(signal(s, handler));

	  case SIGILL:
	  case SIGFPE:
	  case SIGSEGV:
	  case SIGTERM:
	  case SIGABRT:
	    return(signal(s, handler));

	  default:
	    return(SIG_DFL);
	}
}

static void term_event_name(char *buf, size_t size, pid_t pid)
{
	snprintf(buf, size, "Local\\xshipwars-term-%d", (int)pid);
}

/*
 *	Runs in a thread of its own. Windows ends the process when this
 *	returns from closing the console, logging off or shutting down, so
 *	it does not return from those, which leaves the main loop time to
 *	save. Windows only allows some seconds for that, and ends the
 *	process anyway after them.
 */
static BOOL WINAPI console_ctrl(DWORD type)
{
	SetEvent(term_event);

	if((type != CTRL_C_EVENT) && (type != CTRL_BREAK_EVENT))
	    Sleep(INFINITE);

	return(TRUE);
}

void Win32WatchTerm(void)
{
	char name[64];

	term_event_name(name, sizeof(name), GetCurrentProcessId());
	term_event = CreateEventA(NULL, FALSE, FALSE, name);
	if(term_event != NULL)
	    SetConsoleCtrlHandler(console_ctrl, TRUE);
}

void Win32PollTerm(void)
{
	if((term_event != NULL) &&
	   (WaitForSingleObject(term_event, 0) == WAIT_OBJECT_0)
	)
	    raise(SIGTERM);
}

int xsw_kill(pid_t pid, int s)
{
	char name[64];
	HANDLE h;
	int rtn = 0;

	if((s == SIGTERM) || (s == SIGINT))
	{
	    term_event_name(name, sizeof(name), pid);
	    h = OpenEventA(EVENT_MODIFY_STATE, FALSE, name);
	    if(h != NULL)
	    {
		SetEvent(h);
		CloseHandle(h);
		return(0);
	    }
	}

	h = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
	if(h == NULL)
	{
	    errno = (GetLastError() == ERROR_ACCESS_DENIED) ? EPERM : ESRCH;
	    return(-1);
	}
	if((s != 0) && !TerminateProcess(h, 1))
	{
	    errno = EPERM;
	    rtn = -1;
	}
	CloseHandle(h);
	return(rtn);
}


/* Child processes. */
pid_t wait3(int *status, int options, struct rusage *rusage)
{
	errno = ECHILD;		/* Nothing to reap, see Win32Spawn(). */
	return(-1);
}

/* Returns an inheritable handle of the file, or NULL. */
static HANDLE inheritable_handle(FILE *fp)
{
	HANDLE h = NULL;

	if(fp == NULL)
	    return(NULL);
	if(!DuplicateHandle(
	    GetCurrentProcess(), (HANDLE)_get_osfhandle(_fileno(fp)),
	    GetCurrentProcess(), &h, 0, TRUE, DUPLICATE_SAME_ACCESS
	))
	    return(NULL);
	return(h);
}

pid_t Win32Spawn(const char *cmd, FILE *stdout_fp, FILE *stderr_fp, int block)
{
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	HANDLE out = inheritable_handle(stdout_fp);
	HANDLE err = inheritable_handle(stderr_fp);
	std::string cmdline(cmd);	/* CreateProcess() may modify it. */
	pid_t pid = -1;

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	if((out != NULL) || (err != NULL))
	{
	    si.dwFlags = STARTF_USESTDHANDLES;
	    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	    si.hStdOutput = (out != NULL) ? out : GetStdHandle(STD_OUTPUT_HANDLE);
	    si.hStdError = (err != NULL) ? err : GetStdHandle(STD_ERROR_HANDLE);
	}

	if(CreateProcessA(
	    NULL, &cmdline[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi
	))
	{
	    pid = (pid_t)pi.dwProcessId;
	    if(block)
		WaitForSingleObject(pi.hProcess, INFINITE);
	    CloseHandle(pi.hProcess);
	    CloseHandle(pi.hThread);
	}
	else
	{
	    errno = ENOENT;
	}

	if(out != NULL)
	    CloseHandle(out);
	if(err != NULL)
	    CloseHandle(err);
	return(pid);
}


/* fnmatch(3), without the flags (nothing passes any). */
static int match_class(const char **pp, char c)
{
	const char *p = *pp;
	int negate = 0, found = 0;

	if((*p == '!') || (*p == '^'))
	{
	    negate = 1;
	    p++;
	}
	for(int first = 1; first || (*p != ']'); first = 0)
	{
	    if(*p == '\0')
		return(-1);
	    if((p[1] == '-') && (p[2] != ']') && (p[2] != '\0'))
	    {
		if((c >= p[0]) && (c <= p[2]))
		    found = 1;
		p += 3;
	    }
	    else
	    {
		if(c == *p)
		    found = 1;
		p++;
	    }
	}
	*pp = p + 1;
	return(found != negate);
}

int fnmatch(const char *pattern, const char *string, int flags)
{
	const char *p = pattern, *s = string;

	for(; *p != '\0'; s++)
	{
	    switch(*p)
	    {
	      case '*':
		while(*p == '*')
		    p++;
		if(*p == '\0')
		    return(0);
		for(; *s != '\0'; s++)
		    if(!fnmatch(p, s, flags))
			return(0);
		return(FNM_NOMATCH);

	      case '?':
		if(*s == '\0')
		    return(FNM_NOMATCH);
		p++;
		break;

	      case '[':
		{
		    const char *q = p + 1;
		    int status;

		    if(*s == '\0')
			return(FNM_NOMATCH);
		    status = match_class(&q, *s);
		    if(status < 0)
		    {
			/* No closing ], a plain character. */
			if(*s != '[')
			    return(FNM_NOMATCH);
			p++;
		    }
		    else if(!status)
		    {
			return(FNM_NOMATCH);
		    }
		    else
		    {
			p = q;
		    }
		}
		break;

	      case '\\':
		if(p[1] != '\0')
		    p++;
		/* Fall through. */
	      default:
		if(*s != *p)
		    return(FNM_NOMATCH);
		p++;
		break;
	    }
	}
	return((*s == '\0') ? 0 : FNM_NOMATCH);
}


/* POSIX regex, on the POSIX extended grammar of std::regex. */
int regcomp(regex_t *preg, const char *pattern, int cflags)
{
	std::regex::flag_type flags = std::regex::extended;

	if(cflags & REG_ICASE)
	    flags |= std::regex::icase;
	if(cflags & REG_NOSUB)
	    flags |= std::regex::nosubs;

	try
	{
	    preg->re = new std::regex(pattern, flags);
	}
	catch(const std::regex_error &)
	{
	    preg->re = NULL;
	    return(REG_BADPAT);
	}
	return(0);
}

int regexec(
	const regex_t *preg, const char *string,
	size_t nmatch, regmatch_t *pmatch, int eflags
)
{
	return(
	    std::regex_search(string, *(const std::regex *)preg->re) ?
		0 : REG_NOMATCH
	);
}

void regfree(regex_t *preg)
{
	delete (std::regex *)preg->re;
	preg->re = NULL;
}


/*
 *	The traditional DES crypt(3): 25 rounds of DES of a zero block, with
 *	the key from the first 8 characters and the E table perturbed by
 *	the 2 salt characters. The output has to be the same as on UNIX,
 *	since the universe files store it.
 */
static const unsigned char des_ip[64] = {
	58,50,42,34,26,18,10, 2,60,52,44,36,28,20,12, 4,
	62,54,46,38,30,22,14, 6,64,56,48,40,32,24,16, 8,
	57,49,41,33,25,17, 9, 1,59,51,43,35,27,19,11, 3,
	61,53,45,37,29,21,13, 5,63,55,47,39,31,23,15, 7
};
static const unsigned char des_fp[64] = {
	40, 8,48,16,56,24,64,32,39, 7,47,15,55,23,63,31,
	38, 6,46,14,54,22,62,30,37, 5,45,13,53,21,61,29,
	36, 4,44,12,52,20,60,28,35, 3,43,11,51,19,59,27,
	34, 2,42,10,50,18,58,26,33, 1,41, 9,49,17,57,25
};
static const unsigned char des_pc1[56] = {
	57,49,41,33,25,17, 9, 1,58,50,42,34,26,18,
	10, 2,59,51,43,35,27,19,11, 3,60,52,44,36,
	63,55,47,39,31,23,15, 7,62,54,46,38,30,22,
	14, 6,61,53,45,37,29,21,13, 5,28,20,12, 4
};
static const unsigned char des_shifts[16] = {
	1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1
};
static const unsigned char des_pc2[48] = {
	14,17,11,24, 1, 5, 3,28,15, 6,21,10,
	23,19,12, 4,26, 8,16, 7,27,20,13, 2,
	41,52,31,37,47,55,30,40,51,45,33,48,
	44,49,39,56,34,53,46,42,50,36,29,32
};
static const unsigned char des_e[48] = {
	32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9,
	 8, 9,10,11,12,13,12,13,14,15,16,17,
	16,17,18,19,20,21,20,21,22,23,24,25,
	24,25,26,27,28,29,28,29,30,31,32, 1
};
static const unsigned char des_p[32] = {
	16, 7,20,21,29,12,28,17, 1,15,23,26, 5,18,31,10,
	 2, 8,24,14,32,27, 3, 9,19,13,30, 6,22,11, 4,25
};
static const unsigned char des_s[8][64] = {
	{14, 4,13, 1, 2,15,11, 8, 3,10, 6,12, 5, 9, 0, 7,
	  0,15, 7, 4,14, 2,13, 1,10, 6,12,11, 9, 5, 3, 8,
	  4, 1,14, 8,13, 6, 2,11,15,12, 9, 7, 3,10, 5, 0,
	 15,12, 8, 2, 4, 9, 1, 7, 5,11, 3,14,10, 0, 6,13},
	{15, 1, 8,14, 6,11, 3, 4, 9, 7, 2,13,12, 0, 5,10,
	  3,13, 4, 7,15, 2, 8,14,12, 0, 1,10, 6, 9,11, 5,
	  0,14, 7,11,10, 4,13, 1, 5, 8,12, 6, 9, 3, 2,15,
	 13, 8,10, 1, 3,15, 4, 2,11, 6, 7,12, 0, 5,14, 9},
	{10, 0, 9,14, 6, 3,15, 5, 1,13,12, 7,11, 4, 2, 8,
	 13, 7, 0, 9, 3, 4, 6,10, 2, 8, 5,14,12,11,15, 1,
	 13, 6, 4, 9, 8,15, 3, 0,11, 1, 2,12, 5,10,14, 7,
	  1,10,13, 0, 6, 9, 8, 7, 4,15,14, 3,11, 5, 2,12},
	{ 7,13,14, 3, 0, 6, 9,10, 1, 2, 8, 5,11,12, 4,15,
	 13, 8,11, 5, 6,15, 0, 3, 4, 7, 2,12, 1,10,14, 9,
	 10, 6, 9, 0,12,11, 7,13,15, 1, 3,14, 5, 2, 8, 4,
	  3,15, 0, 6,10, 1,13, 8, 9, 4, 5,11,12, 7, 2,14},
	{ 2,12, 4, 1, 7,10,11, 6, 8, 5, 3,15,13, 0,14, 9,
	 14,11, 2,12, 4, 7,13, 1, 5, 0,15,10, 3, 9, 8, 6,
	  4, 2, 1,11,10,13, 7, 8,15, 9,12, 5, 6, 3, 0,14,
	 11, 8,12, 7, 1,14, 2,13, 6,15, 0, 9,10, 4, 5, 3},
	{12, 1,10,15, 9, 2, 6, 8, 0,13, 3, 4,14, 7, 5,11,
	 10,15, 4, 2, 7,12, 9, 5, 6, 1,13,14, 0,11, 3, 8,
	  9,14,15, 5, 2, 8,12, 3, 7, 0, 4,10, 1,13,11, 6,
	  4, 3, 2,12, 9, 5,15,10,11,14, 1, 7, 6, 0, 8,13},
	{ 4,11, 2,14,15, 0, 8,13, 3,12, 9, 7, 5,10, 6, 1,
	 13, 0,11, 7, 4, 9, 1,10,14, 3, 5,12, 2,15, 8, 6,
	  1, 4,11,13,12, 3, 7,14,10,15, 6, 8, 0, 5, 9, 2,
	  6,11,13, 8, 1, 4,10, 7, 9, 5, 0,15,14, 2, 3,12},
	{13, 2, 8, 4, 6,15,11, 1,10, 9, 3,14, 5, 0,12, 7,
	  1,15,13, 8,10, 3, 7, 4,12, 5, 6,11, 0,14, 9, 2,
	  7,11, 4, 1, 9,12,14, 2, 0, 6,10,13,15, 3, 5, 8,
	  2, 1,14, 7, 4,10, 8,13,15,12, 9, 0, 3, 5, 6,11}
};

char *crypt(const char *key, const char *salt)
{
	static char out[14];
	unsigned char keybits[64], cd[56], subkey[16][48];
	unsigned char e[48], block[66];
	int i, j, k;

	/* Key bits: 7 bits of each of the first 8 characters. */
	memset(keybits, 0, sizeof(keybits));
	for(i = 0; (i < 8) && (key[i] != '\0'); i++)
	    for(j = 0; j < 7; j++)
		keybits[i * 8 + j] = (key[i] >> (6 - j)) & 1;

	/* Key schedule. */
	for(i = 0; i < 56; i++)
	    cd[i] = keybits[des_pc1[i] - 1];
	for(i = 0; i < 16; i++)
	{
	    for(k = 0; k < des_shifts[i]; k++)
	    {
		unsigned char c0 = cd[0], d0 = cd[28];

		memmove(cd, cd + 1, 27);
		cd[27] = c0;
		memmove(cd + 28, cd + 29, 27);
		cd[55] = d0;
	    }
	    for(j = 0; j < 48; j++)
		subkey[i][j] = cd[des_pc2[j] - 1];
	}

	/* The salt swaps entries of the E table. */
	memcpy(e, des_e, sizeof(e));
	for(i = 0; i < 2; i++)
	{
	    int c = (unsigned char)salt[i];

	    out[i] = (char)c;
	    if(c > 'Z')
		c -= 6;
	    if(c > '9')
		c -= 7;
	    c -= '.';
	    for(j = 0; j < 6; j++)
	    {
		if((c >> j) & 1)
		{
		    unsigned char t = e[6 * i + j];

		    e[6 * i + j] = e[6 * i + j + 24];
		    e[6 * i + j + 24] = t;
		}
	    }
	}

	/* 25 encryptions of the block. */
	memset(block, 0, sizeof(block));
	for(int round = 0; round < 25; round++)
	{
	    unsigned char lr[64], tmp[32], f[32], ex[48];
	    unsigned char *l = lr, *r = lr + 32;

	    for(i = 0; i < 64; i++)
		lr[i] = block[des_ip[i] - 1];

	    for(i = 0; i < 16; i++)
	    {
		memcpy(tmp, r, 32);

		for(j = 0; j < 48; j++)
		    ex[j] = r[e[j] - 1] ^ subkey[i][j];

		for(j = 0; j < 8; j++)
		{
		    const unsigned char *b = ex + j * 6;
		    int row = (b[0] << 1) | b[5];
		    int col = (b[1] << 3) | (b[2] << 2) | (b[3] << 1) | b[4];
		    int v = des_s[j][row * 16 + col];

		    f[j * 4 + 0] = (v >> 3) & 1;
		    f[j * 4 + 1] = (v >> 2) & 1;
		    f[j * 4 + 2] = (v >> 1) & 1;
		    f[j * 4 + 3] = v & 1;
		}

		for(j = 0; j < 32; j++)
		    r[j] = l[j] ^ f[des_p[j] - 1];

		memcpy(l, tmp, 32);
	    }

	    /* Swap the halves, then the final permutation. */
	    {
		unsigned char sw[64];

		memcpy(sw, r, 32);
		memcpy(sw + 32, l, 32);
		for(i = 0; i < 64; i++)
		    block[i] = sw[des_fp[i] - 1];
	    }
	}

	/* 6 bits per output character. */
	for(i = 0; i < 11; i++)
	{
	    int c = 0;

	    for(j = 0; j < 6; j++)
		c = (c << 1) | block[6 * i + j];
	    c += '.';
	    if(c > '9')
		c += 7;
	    if(c > 'Z')
		c += 6;
	    out[i + 2] = (char)c;
	}
	out[13] = '\0';
	if(out[1] == '\0')
	    out[1] = out[0];

	return(out);
}
