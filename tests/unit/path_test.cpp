// tests/unit/path_test.cpp
/*
	The path functions of global/disk.cpp and of the file browser, and
	how a configuration file line with \ in it is read.

	UNIX paths have to give the same results everywhere. The drive
	letter cases (C:/dir) only apply to Windows.
 */
// unittest-sources: global/disk.cpp global/string.cpp global/fio.cpp global/strexp.cpp
// unittest-extract: widgets/wfbrowser.cpp FBrowserHasMask FBrowserGetJustPath

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../../src/include/disk.h"
#include "../../src/include/fio.h"
#include "unittest.h"

/* FBrowserHasMask() and FBrowserGetJustPath(), taken from the source. */
#include "extract.inc"


/* For the functions that change their argument. */
static char buf[1024];

static const char *simplify(const char *path)
{
	strcpy(buf, path);
	SimplifyPath(buf);
	return(buf);
}

static const char *strip_abs(const char *path)
{
	strcpy(buf, path);
	StripAbsolutePath(buf);
	return(buf);
}

static const char *strip_parent(const char *path, const char *parent)
{
	strcpy(buf, path);
	StripParentPath(buf, parent);
	return(buf);
}

static const char *chdir_rel(const char *cpath, const char *npath)
{
	char *s = ChangeDirRel(cpath, npath);

	strcpy(buf, (s != NULL) ? s : "(null)");
	free(s);
	return(buf);
}

static const char *just_path(const char *path)
{
	char tmp[1024];

	strcpy(tmp, path);
	return(FBrowserGetJustPath(tmp));
}


int main(int argc, char *argv[])
{
	/* UNIX paths. */
	CHECK_INT(ISPATHABSOLUTE("/a"), 1);
	CHECK_INT(ISPATHABSOLUTE("  /a"), 1);
	CHECK_INT(ISPATHABSOLUTE("a/b"), 0);
	CHECK_STR(PrefixPaths("/a", "b"), "/a/b");
	CHECK_STR(PrefixPaths("/a/", "b"), "/a/b");
	CHECK_STR(PrefixPaths("/a", "/b"), "/b");
	CHECK_STR(GetParentDir("/a/b"), "/a");
	CHECK_STR(GetParentDir("/a/b/"), "/a");
	CHECK_STR(GetParentDir("/a"), "/");
	CHECK_STR(simplify("/a/b/.."), "/a");
	CHECK_STR(simplify("/a/b/../c"), "/a/c");
	CHECK_STR(simplify("/a/.."), "/");
	CHECK_STR(strip_abs("/a/b"), "b");
	CHECK_INT(COMPARE_PARENT_PATHS("/a/b", "/a"), 1);
	CHECK_INT(COMPARE_PARENT_PATHS("/b/c", "/a"), 0);
	CHECK_STR(strip_parent("/a/b", "/a"), "b");
	CHECK_STR(chdir_rel("/a", "b"), "/a/b");
	CHECK_STR(chdir_rel("/a/b", ".."), "/a");
	CHECK_STR(chdir_rel("/a", "/c"), "/c");
	CHECK_STR(just_path("/a/*.unv"), "/a");
	CHECK_STR(just_path("/*.unv"), "/");
	CHECK_STR(just_path("/a/b"), "/a/b");

#ifdef _WIN32
	/* Drive letters. Only / is a separator. */
	CHECK_INT(ISPATHABSOLUTE("C:/a"), 1);
	CHECK_INT(ISPATHABSOLUTE("c:/a"), 1);
	CHECK_INT(ISPATHABSOLUTE("C:\\a"), 0);
	CHECK_INT(ISPATHABSOLUTE("C:a"), 0);	/* Relative to the drive's cwd. */
	CHECK_STR(PrefixPaths("C:/a", "b"), "C:/a/b");
	CHECK_STR(PrefixPaths("C:/", "b"), "C:/b");
	CHECK_STR(PrefixPaths("C:/a", "D:/b"), "D:/b");
	CHECK_STR(GetParentDir("C:/a/b"), "C:/a");
	CHECK_STR(GetParentDir("C:/a"), "C:/");
	CHECK_STR(GetParentDir("C:/"), "C:/");
	CHECK_STR(simplify("C:/a/b/.."), "C:/a");
	CHECK_STR(simplify("C:/a/.."), "C:/");
	CHECK_STR(simplify("C:/.."), "C:/");
	CHECK_STR(strip_abs("C:/a/b"), "b");
	CHECK_INT(COMPARE_PARENT_PATHS("C:/a/b", "C:/a"), 1);
	CHECK_INT(COMPARE_PARENT_PATHS("D:/a/b", "C:/a"), 0);
	CHECK_STR(strip_parent("C:/a/b", "C:/a"), "b");
	CHECK_STR(chdir_rel("C:/a", "b"), "C:/a/b");
	CHECK_STR(chdir_rel("C:/a/b", ".."), "C:/a");
	CHECK_STR(chdir_rel("C:/a", ".."), "C:/");
	CHECK_STR(chdir_rel("C:/a", "D:/x"), "D:/x");
	CHECK_STR(just_path("C:/a/*.unv"), "C:/a");
	CHECK_STR(just_path("C:/*.unv"), "C:/");
#endif

	/*   A line of a configuration file. \ is an escape character there
	 *   (on every system), so paths in them are written with / (a \
	 *   would have to be written \\).
	 */
	if(argc > 1)
	{
	    char path[1024], *line;
	    FILE *fp;

	    snprintf(path, sizeof(path), "%s/config.rc", argv[1]);
	    fp = fopen(path, "wb");
	    fputs("ImagesDir = C:\\\\xsw\\\\images\n", fp);	/* C:\\xsw\\images */
	    fputs("ImagesDir = C:\\xsw\n", fp);		/* C:\xsw */
	    fclose(fp);

	    fp = fopen(path, "rb");
	    line = FReadNextLineAlloc(fp, '#');
	    CHECK_STR(line, "ImagesDir = C:\\xsw\\images\n");
	    free(line);
	    line = FReadNextLineAlloc(fp, '#');
	    CHECK_STR(line, "ImagesDir = C:xsw\n");	/* A lone \ is lost. */
	    free(line);
	    fclose(fp);
	}

	/* rmkdir() creates every missing level (argv[1] is a work dir). */
	if(argc > 1)
	{
	    char path[1024];
	    struct stat st;

	    snprintf(path, sizeof(path), "%s/rmkdir/x/y", argv[1]);
	    CHECK_INT(rmkdir(path, 0755), 0);
	    CHECK_INT((stat(path, &st) == 0) && S_ISDIR(st.st_mode), 1);
	}

	return(UNITTEST_END());
}
