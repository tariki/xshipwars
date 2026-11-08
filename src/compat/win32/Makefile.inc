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
