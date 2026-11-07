# ########################################################################
#
#   Shared part of the src/*/Makefile.Windows files.
#
#   Build in the UCRT64 environment of MSYS2. The calling Makefile sets
#   CFLAGS, LIB and BIN_NAME and includes Makefile.srclist first.
#
#   The programs have their own main() and stay console programs, so the
#   GUI programs leave out the parts of SDL2's flags that replace main()
#   and hide the console (SDL_FILTER in their Makefiles).
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
LIB += -lws2_32 -lwinmm

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
