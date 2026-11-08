# ########################################################################
#
#   Shared part of the src/*/Makefile.Windows files.
#
#   Build in the UCRT64 environment of MSYS2. The calling Makefile sets
#   CFLAGS, LIB and BIN_NAME and includes Makefile.srclist first.
#
#   The GUI programs define XSW_WIN32_GUI and link with -mwindows, so that
#   no console window opens with them (see compat.cpp). They have their
#   own main(), so they leave out the parts of SDL2's flags that replace
#   it (SDL_FILTER in their Makefiles). The server stays a console
#   program.
#
#   Objects and the program are built in $(OBJDIR), so that they do not
#   mix with the other builds in the same source tree.
#

PREFIX = C:/xshipwars

COMPAT = ../compat/win32

#   Installation directories compiled in (see include/xsw-paths.h).
CFLAGS += -O2 -g -Wall -include compat.h \
	-DXSW_DATA_DIR=\"$(PREFIX)/share/games/xshipwars\" \
	-DSWSERV_DIR=\"$(PREFIX)/swserv\"

INC = -I$(COMPAT)
#   The default stack is 1 MB. The programs have many path buffers on the
#   stack, so give them the 8 MB that they have on Linux.
LIB += -lws2_32 -lwinmm -Wl,--stack,8388608

CPP    = g++
OBJDIR = build-windows
BIN    = $(OBJDIR)/$(BIN_NAME).exe

vpath compat.cpp $(COMPAT)
SRC += compat.cpp
OBJ  = $(addprefix $(OBJDIR)/,$(SRC:.cpp=.o))

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	$(CPP) -c $< -o $@ $(INC) $(CFLAGS)

$(BIN): $(OBJ)
	$(CPP) $(OBJ) -o $(BIN) $(LIB)

all: $(BIN)

clean:
	rm -rf $(OBJDIR)


#   Installation:
#
#   The calling Makefile includes its Makefile.install.UNIX after this
#   file (so that "all" stays the default), which installs as on UNIX
#   (programs in $(PREFIX)/bin, the server in $(PREFIX)/swserv). This adds
#   the DLLs from MSYS2 that the program uses next to it (in $(DLL_DIR)),
#   so that it runs without MSYS2 in the PATH.
#
#   ldd only lists the DLLs that the program is linked with. RUNTIME_DLLS
#   names the ones that are loaded by name while it runs (SDL2_mixer loads
#   libfluidsynth-3.dll for MIDI), which are copied with their own DLLs.
MINGW_PREFIX ?= /ucrt64

install: install-dlls

install-dlls: $(BIN)
	mkdir -p $(DLL_DIR)
	for f in $(BIN) $(addprefix $(MINGW_PREFIX)/bin/,$(RUNTIME_DLLS)); do \
	    echo "$$f" | grep -v '\.exe$$'; \
	    ldd "$$f" | awk '$$3 ~ "^$(MINGW_PREFIX)/" {print $$3}'; \
	done | sort -u | while read -r d; do \
	    cp -u -v "$$d" "$(DLL_DIR)/"; \
	done

.PHONY: all clean install install-dlls
