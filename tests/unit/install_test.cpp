// tests/unit/install_test.cpp
/*
	XSWDoInstallObject() of the client, which makes ~/.shipwars and
	copies the configuration files into it on the first start.

	The work directory (argv[1]) is an absolute path, so on Windows this
	goes through a drive letter (C:/...).
 */
// unittest-sources: global/disk.cpp global/string.cpp global/fio.cpp global/strexp.cpp
// unittest-extract: client/xswinstall.cpp XSWDoInstallObject

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>		/* What xswinstall.cpp includes for the */
#include <unistd.h>		/* function taken from it (getcwd()). */

#include "../../src/include/disk.h"
#include "../../src/client/xswinstall.h"
#include "unittest.h"

#include "extract.inc"


static int is_dir(const char *path)
{
	struct stat st;

	return((stat(path, &st) == 0) && S_ISDIR(st.st_mode));
}

static char *read_file(const char *path, char *buf, size_t size)
{
	FILE *fp = fopen(path, "rb");
	size_t n = 0;

	if(fp != NULL)
	{
	    n = fread(buf, 1, size - 1, fp);
	    fclose(fp);
	}
	buf[n] = '\0';
	return(buf);
}


int main(int argc, char *argv[])
{
	char src[1024], dir[1024], buf[256];
	char target[sizeof(dir) + 16];	/* dir and "/xshipwarsrc". */
	FILE *fp;

	if(argc < 2)
	{
	    printf("usage: %s <work dir>\n", argv[0]);
	    return(2);
	}

	/* A configuration file to install. */
	snprintf(src, sizeof(src), "%s/xshipwarsrc", argv[1]);
	fp = fopen(src, "wb");
	fputs("ToplevelDir = /usr/share/games/xshipwars\n", fp);
	fclose(fp);

	/* Like the client: the directory (with its parents), then the file. */
	snprintf(dir, sizeof(dir), "%s/home/user/.shipwars", argv[1]);
	CHECK_INT(XSWDoInstallObject(
	    dir, src, S_IRUSR | S_IWUSR | S_IXUSR,
	    XSW_INST_OBJ_FLAG_DIR | XSW_INST_OBJ_FLAG_RECURSIVE, NULL
	), 0);
	CHECK_INT(is_dir(dir), 1);

	snprintf(target, sizeof(target), "%s/xshipwarsrc", dir);
	CHECK_INT(XSWDoInstallObject(target, src, S_IRUSR | S_IWUSR, 0, NULL), 0);
	CHECK_STR(read_file(target, buf, sizeof(buf)),
	    "ToplevelDir = /usr/share/games/xshipwars\n");

	/* Making a directory that already exists is not an error. */
	CHECK_INT(XSWDoInstallObject(
	    dir, src, S_IRUSR | S_IWUSR | S_IXUSR,
	    XSW_INST_OBJ_FLAG_DIR | XSW_INST_OBJ_FLAG_RECURSIVE, NULL
	), 0);

	return(UNITTEST_END());
}
