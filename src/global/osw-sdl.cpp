// global/osw-sdl.cpp
/*
		Operating System Wrapper for SDL2

	The SDL2 implementation of the OSW layer, used instead of
	osw-x.cpp when built with GUI=sdl (OSW_SDL defined). See
	include/osw-sdl.h for the data types.

	The rest of the program was written for X, so this file
	reproduces the parts of X it relies on:

	- Windows form a tree as in X. Only toplevel windows (children
	  of the root window) are SDL windows; all other windows exist
	  only here. Every window and pixmap has its own 32 bit buffer
	  (0x00RRGGBB, like a 24 bit TrueColor X server), and the windows
	  of a toplevel are composited in stacking order and shown when
	  events are checked (OSWEventsPending()) or on OSWGUISync().
	  Window contents are kept (like X's backing store), so Expose
	  is only sent when a window becomes viewable or grows.

	- SDL events are turned into X events and delivered by X's
	  rules: pointer and key events go to the innermost window that
	  selected them, starting at the window under the pointer;
	  a window that got a ButtonPress gets the pointer events until
	  all buttons are released; mapping a window sends MapNotify,
	  Expose and VisibilityNotify; closing a toplevel sends a
	  WM_DELETE_WINDOW ClientMessage.

	- Keycodes are X's (evdev) keycodes, so that the keymaps in the
	  configuration files work unchanged. The keysyms of each
	  keycode are those of an evdev X server with the us layout
	  (taken from Xvfb), and are used in the same way as osw-x.cpp
	  does to find keycodes, key names and characters. Characters
	  typed come from SDL's text input when there is any.

	- Fonts are the X misc-fixed fonts 7x14, 6x10 and 6x12, built
	  in (include/osw-sdl-fonts.h).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <deque>
#include <string>
#include <vector>

#include <SDL.h>

#include "../include/os.h"
#include "../include/string.h"
#include "../include/osw-x.h"
#include "../include/graphics.h"
#include "../include/osw-sdl-fonts.h"

#ifndef MAX
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif


osw_gui_struct osw_gui[1];
osw_keycode_struct osw_keycode;
osw_atom_struct osw_atom;

/*   Color lookup tables used by the blitting code for 15 and 16 bit
 *   depths (graphics.h). The SDL2 GUI is always 24 bit, so they are
 *   not filled.
 */
u_int16_t gRLookup[256];
u_int16_t gGLookup[256];
u_int16_t gBLookup[256];


namespace static_osw_sdl {

/* ******************************************************************
 *
 *	Keyboard map of an evdev X server with the us layout (keycode,
 *	keysym names of the first two levels). Taken from Xvfb with
 *	XGetKeyboardMapping().
 */
struct keymap_entry {
	unsigned int keycode;
	const char *sym[2];
};
static const keymap_entry keymap[] = {
	{9, {"Escape", NULL}},
	{10, {"1", "exclam"}},
	{11, {"2", "at"}},
	{12, {"3", "numbersign"}},
	{13, {"4", "dollar"}},
	{14, {"5", "percent"}},
	{15, {"6", "asciicircum"}},
	{16, {"7", "ampersand"}},
	{17, {"8", "asterisk"}},
	{18, {"9", "parenleft"}},
	{19, {"0", "parenright"}},
	{20, {"minus", "underscore"}},
	{21, {"equal", "plus"}},
	{22, {"BackSpace", "BackSpace"}},
	{23, {"Tab", "ISO_Left_Tab"}},
	{24, {"q", "Q"}},
	{25, {"w", "W"}},
	{26, {"e", "E"}},
	{27, {"r", "R"}},
	{28, {"t", "T"}},
	{29, {"y", "Y"}},
	{30, {"u", "U"}},
	{31, {"i", "I"}},
	{32, {"o", "O"}},
	{33, {"p", "P"}},
	{34, {"bracketleft", "braceleft"}},
	{35, {"bracketright", "braceright"}},
	{36, {"Return", NULL}},
	{37, {"Control_L", NULL}},
	{38, {"a", "A"}},
	{39, {"s", "S"}},
	{40, {"d", "D"}},
	{41, {"f", "F"}},
	{42, {"g", "G"}},
	{43, {"h", "H"}},
	{44, {"j", "J"}},
	{45, {"k", "K"}},
	{46, {"l", "L"}},
	{47, {"semicolon", "colon"}},
	{48, {"apostrophe", "quotedbl"}},
	{49, {"grave", "asciitilde"}},
	{50, {"Shift_L", NULL}},
	{51, {"backslash", "bar"}},
	{52, {"z", "Z"}},
	{53, {"x", "X"}},
	{54, {"c", "C"}},
	{55, {"v", "V"}},
	{56, {"b", "B"}},
	{57, {"n", "N"}},
	{58, {"m", "M"}},
	{59, {"comma", "less"}},
	{60, {"period", "greater"}},
	{61, {"slash", "question"}},
	{62, {"Shift_R", NULL}},
	{63, {"KP_Multiply", "KP_Multiply"}},
	{64, {"Alt_L", "Meta_L"}},
	{65, {"space", NULL}},
	{66, {"Caps_Lock", NULL}},
	{67, {"F1", "F1"}},
	{68, {"F2", "F2"}},
	{69, {"F3", "F3"}},
	{70, {"F4", "F4"}},
	{71, {"F5", "F5"}},
	{72, {"F6", "F6"}},
	{73, {"F7", "F7"}},
	{74, {"F8", "F8"}},
	{75, {"F9", "F9"}},
	{76, {"F10", "F10"}},
	{77, {"Num_Lock", NULL}},
	{78, {"Scroll_Lock", NULL}},
	{79, {"KP_Home", "KP_7"}},
	{80, {"KP_Up", "KP_8"}},
	{81, {"KP_Prior", "KP_9"}},
	{82, {"KP_Subtract", "KP_Subtract"}},
	{83, {"KP_Left", "KP_4"}},
	{84, {"KP_Begin", "KP_5"}},
	{85, {"KP_Right", "KP_6"}},
	{86, {"KP_Add", "KP_Add"}},
	{87, {"KP_End", "KP_1"}},
	{88, {"KP_Down", "KP_2"}},
	{89, {"KP_Next", "KP_3"}},
	{90, {"KP_Insert", "KP_0"}},
	{91, {"KP_Delete", "KP_Decimal"}},
	{92, {"ISO_Level3_Shift", NULL}},
	{94, {"less", "greater"}},
	{95, {"F11", "F11"}},
	{96, {"F12", "F12"}},
	{98, {"Katakana", NULL}},
	{99, {"Hiragana", NULL}},
	{100, {"Henkan_Mode", NULL}},
	{101, {"Hiragana_Katakana", NULL}},
	{102, {"Muhenkan", NULL}},
	{104, {"KP_Enter", NULL}},
	{105, {"Control_R", NULL}},
	{106, {"KP_Divide", "KP_Divide"}},
	{107, {"Print", "Sys_Req"}},
	{108, {"Alt_R", "Meta_R"}},
	{109, {"Linefeed", NULL}},
	{110, {"Home", NULL}},
	{111, {"Up", NULL}},
	{112, {"Prior", NULL}},
	{113, {"Left", NULL}},
	{114, {"Right", NULL}},
	{115, {"End", NULL}},
	{116, {"Down", NULL}},
	{117, {"Next", NULL}},
	{118, {"Insert", NULL}},
	{119, {"Delete", NULL}},
	{121, {"XF86AudioMute", NULL}},
	{122, {"XF86AudioLowerVolume", NULL}},
	{123, {"XF86AudioRaiseVolume", NULL}},
	{124, {"XF86PowerOff", NULL}},
	{125, {"KP_Equal", NULL}},
	{126, {"plusminus", NULL}},
	{127, {"Pause", "Break"}},
	{128, {"XF86LaunchA", NULL}},
	{129, {"KP_Decimal", "KP_Decimal"}},
	{130, {"Hangul", NULL}},
	{131, {"Hangul_Hanja", NULL}},
	{133, {"Super_L", NULL}},
	{134, {"Super_R", NULL}},
	{135, {"Menu", NULL}},
};

/*
 *	Keysym names to characters, the same table as osw-x.cpp's
 *	(the names are those of XKeysymToString()).
 */
struct keychar_entry {
	const char *name;
	char c;
};
static const keychar_entry keychar[] = {
	{"space", ' '}, {"exclam", '!'}, {"quotedbl", '"'},
	{"numbersign", '#'}, {"dollar", '$'}, {"percent", '%'},
	{"ampersand", '&'}, {"apostrophe", '\''}, {"quoteright", '"'},
	{"parenleft", '('}, {"parenright", ')'}, {"asterisk", '*'},
	{"plus", '+'}, {"comma", ','}, {"minus", '-'}, {"period", '.'},
	{"slash", '/'}, {"colon", ':'}, {"semicolon", ';'}, {"less", '<'},
	{"equal", '='}, {"greater", '>'}, {"question", '?'}, {"at", '@'},
	{"bracketleft", '['}, {"backslash", '\\'}, {"bracketright", ']'},
	{"asciicircum", '^'}, {"underscore", '_'}, {"grave", '\0'},
	{"quoteleft", '"'}, {"braceleft", '{'}, {"bar", '|'},
	{"braceright", '}'}, {"asciitilde", '~'},
	{"tab", '\t'}
};

/*
 *	Keysym names to the names OSWGetKeyCodeName() returns, as in
 *	osw-x.cpp. Letters and digits return themselves.
 */
struct keyname_entry {
	const char *sym, *name;
};
static const keyname_entry keyname[] = {
	{"BackSpace", "backspace"}, {"Tab", "tab"}, {"Linefeed", "linefeed"},
	{"Clear", "clear"}, {"Return", "enter"}, {"Pause", "pause"},
	{"Scroll_Lock", "scroll lock"}, {"Sys_Req", "system request"},
	{"Escape", "escape"}, {"Delete", "delete"}, {"Home", "home"},
	{"End", "end"}, {"Prior", "page up"}, {"Next", "page down"},
	{"Up", "up"}, {"Down", "down"}, {"Left", "left"}, {"Right", "right"},
	{"Insert", "insert"}, {"Break", "break"}, {"Num_Lock", "num lock"},
	{"KP_Space", "np space"}, {"KP_Tab", "np tab"},
	{"KP_Enter", "np enter"}, {"KP_Home", "np home"},
	{"KP_End", "np end"}, {"KP_Prior", "np page up"},
	{"KP_Next", "np page down"}, {"KP_Up", "np up"},
	{"KP_Down", "np down"}, {"KP_Left", "np left"},
	{"KP_Right", "np right"}, {"KP_Insert", "np insert"},
	{"KP_Delete", "np delete"}, {"KP_Equal", "np equal"},
	{"KP_Multiply", "np multiply"}, {"KP_Divide", "np divide"},
	{"KP_Add", "np add"}, {"KP_Subtract", "np subtract"},
	{"KP_Decimal", "np decimal"},
	{"KP_0", "np 0"}, {"KP_1", "np 1"}, {"KP_2", "np 2"},
	{"KP_3", "np 3"}, {"KP_4", "np 4"}, {"KP_5", "np 5"},
	{"KP_6", "np 6"}, {"KP_7", "np 7"}, {"KP_8", "np 8"},
	{"KP_9", "np 9"},
	{"F1", "f1"}, {"F2", "f2"}, {"F3", "f3"}, {"F4", "f4"},
	{"F5", "f5"}, {"F6", "f6"}, {"F7", "f7"}, {"F8", "f8"},
	{"F9", "f9"}, {"F10", "f10"}, {"F11", "f11"}, {"F12", "f12"},
	{"Shift_L", "shift left"}, {"Shift_R", "shift right"},
	{"Control_L", "control left"}, {"Control_R", "control right"},
	{"Caps_Lock", "caps lock"}, {"Alt_L", "alt left"},
	{"Alt_R", "alt right"}, {"space", "space"}, {"minus", "minus"},
	{"equal", "equal"}, {"backslash", "back slash"},
	{"grave", "quote left"}, {"bracketleft", "bracket left"},
	{"bracketright", "bracket right"}, {"braceleft", "brace left"},
	{"braceright", "brace right"}, {"semicolon", "semicolon"},
	{"apostrophe", "quote right"}, {"comma", "comma"},
	{"period", "period"}, {"slash", "slash"}
};

/*
 *	SDL scancode to X (evdev) keycode, 0 if none.
 */
static unsigned int ScancodeToKeycode(SDL_Scancode sc)
{
	/* Letters a to z. */
	static const unsigned char alpha[26] = {
	    38, 56, 54, 40, 26, 41, 42, 43, 31, 44, 45, 46, 58,
	    57, 32, 33, 24, 27, 39, 28, 30, 55, 25, 53, 29, 52
	};
	/* Keypad 1 to 9. */
	static const unsigned char kp[9] = {
	    87, 88, 89, 83, 84, 85, 79, 80, 81
	};

	if((sc >= SDL_SCANCODE_A) && (sc <= SDL_SCANCODE_Z))
	    return(alpha[sc - SDL_SCANCODE_A]);
	if((sc >= SDL_SCANCODE_1) && (sc <= SDL_SCANCODE_0))
	    return(10 + (sc - SDL_SCANCODE_1));
	if((sc >= SDL_SCANCODE_F1) && (sc <= SDL_SCANCODE_F10))
	    return(67 + (sc - SDL_SCANCODE_F1));
	if((sc >= SDL_SCANCODE_KP_1) && (sc <= SDL_SCANCODE_KP_9))
	    return(kp[sc - SDL_SCANCODE_KP_1]);
	if((sc >= SDL_SCANCODE_F13) && (sc <= SDL_SCANCODE_F24))
	    return(191 + (sc - SDL_SCANCODE_F13));

	switch(sc)
	{
	  case SDL_SCANCODE_RETURN: return(36);
	  case SDL_SCANCODE_ESCAPE: return(9);
	  case SDL_SCANCODE_BACKSPACE: return(22);
	  case SDL_SCANCODE_TAB: return(23);
	  case SDL_SCANCODE_SPACE: return(65);
	  case SDL_SCANCODE_MINUS: return(20);
	  case SDL_SCANCODE_EQUALS: return(21);
	  case SDL_SCANCODE_LEFTBRACKET: return(34);
	  case SDL_SCANCODE_RIGHTBRACKET: return(35);
	  case SDL_SCANCODE_BACKSLASH: return(51);
	  case SDL_SCANCODE_NONUSHASH: return(51);
	  case SDL_SCANCODE_SEMICOLON: return(47);
	  case SDL_SCANCODE_APOSTROPHE: return(48);
	  case SDL_SCANCODE_GRAVE: return(49);
	  case SDL_SCANCODE_COMMA: return(59);
	  case SDL_SCANCODE_PERIOD: return(60);
	  case SDL_SCANCODE_SLASH: return(61);
	  case SDL_SCANCODE_CAPSLOCK: return(66);
	  case SDL_SCANCODE_F11: return(95);
	  case SDL_SCANCODE_F12: return(96);
	  case SDL_SCANCODE_PRINTSCREEN: return(107);
	  case SDL_SCANCODE_SCROLLLOCK: return(78);
	  case SDL_SCANCODE_PAUSE: return(127);
	  case SDL_SCANCODE_INSERT: return(118);
	  case SDL_SCANCODE_HOME: return(110);
	  case SDL_SCANCODE_PAGEUP: return(112);
	  case SDL_SCANCODE_DELETE: return(119);
	  case SDL_SCANCODE_END: return(115);
	  case SDL_SCANCODE_PAGEDOWN: return(117);
	  case SDL_SCANCODE_RIGHT: return(114);
	  case SDL_SCANCODE_LEFT: return(113);
	  case SDL_SCANCODE_DOWN: return(116);
	  case SDL_SCANCODE_UP: return(111);
	  case SDL_SCANCODE_NUMLOCKCLEAR: return(77);
	  case SDL_SCANCODE_KP_DIVIDE: return(106);
	  case SDL_SCANCODE_KP_MULTIPLY: return(63);
	  case SDL_SCANCODE_KP_MINUS: return(82);
	  case SDL_SCANCODE_KP_PLUS: return(86);
	  case SDL_SCANCODE_KP_ENTER: return(104);
	  case SDL_SCANCODE_KP_0: return(90);
	  case SDL_SCANCODE_KP_PERIOD: return(91);
	  case SDL_SCANCODE_KP_EQUALS: return(125);
	  case SDL_SCANCODE_NONUSBACKSLASH: return(94);
	  case SDL_SCANCODE_APPLICATION: return(135);
	  case SDL_SCANCODE_INTERNATIONAL1: return(97);	/* Japanese ro. */
	  case SDL_SCANCODE_INTERNATIONAL2: return(101);	/* Katakana/hiragana. */
	  case SDL_SCANCODE_INTERNATIONAL3: return(132);	/* Yen. */
	  case SDL_SCANCODE_INTERNATIONAL4: return(100);	/* Henkan. */
	  case SDL_SCANCODE_INTERNATIONAL5: return(102);	/* Muhenkan. */
	  case SDL_SCANCODE_LANG1: return(130);
	  case SDL_SCANCODE_LANG2: return(131);
	  case SDL_SCANCODE_LCTRL: return(37);
	  case SDL_SCANCODE_LSHIFT: return(50);
	  case SDL_SCANCODE_LALT: return(64);
	  case SDL_SCANCODE_LGUI: return(133);
	  case SDL_SCANCODE_RCTRL: return(105);
	  case SDL_SCANCODE_RSHIFT: return(62);
	  case SDL_SCANCODE_RALT: return(108);
	  case SDL_SCANCODE_RGUI: return(134);
	  default: return(0);
	}
}

/* Keysym name of keycode at level (0 or 1), NULL if none. */
static const char *KeycodeSym(unsigned int keycode, int level)
{
	size_t i;

	for(i = 0; i < sizeof(keymap) / sizeof(keymap[0]); i++)
	{
	    if(keymap[i].keycode == keycode)
		return(keymap[i].sym[level]);
	}
	return(NULL);
}

/*   Keycode of the keysym name like XKeysymToKeycode(): the lowest
 *   keycode with it at level 0, else at level 1, else 0.
 */
static keycode_t SymToKeycode(const char *sym)
{
	int level;
	size_t i;

	for(level = 0; level < 2; level++)
	{
	    for(i = 0; i < sizeof(keymap) / sizeof(keymap[0]); i++)
	    {
		if((keymap[i].sym[level] != NULL) &&
		   !strcmp(keymap[i].sym[level], sym)
		)
		    return(keymap[i].keycode);
	    }
	}
	return(0);
}


/* ******************************************************************
 *
 *	Windows, pixmaps and cursors.
 */
enum {
	OBJ_FREE = 0,
	OBJ_WINDOW,
	OBJ_PIXMAP
};

struct obj {
	int type;
	int width, height;
	u_int32_t *data;	/* NULL for input only windows and root. */

	/* Windows. */
	win_t parent;
	std::vector<win_t> children;	/* Bottom to top. */
	int x, y;		/* Relative to the parent (toplevels: screen). */
	bool mapped;
	bool input_only;
	long event_mask;
	cursor_t cursor;
	pixel_t bkg_pix;
	pixmap_t bkg_pixmap;

	/* Toplevel windows. */
	SDL_Window *sdl_win;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	int tex_width, tex_height;
	u_int32_t *comp;	/* Composited contents. */
	bool dirty;
	bool iconified;
	int frame_style;
	unsigned int min_width, min_height, max_width, max_height;
	std::string title;
	pixmap_t icon;
};

static std::vector<obj *> objs;		/* Handle is the index, 1 is root. */
static std::vector<SDL_Cursor *> cursors;	/* Handle is the index + 1. */

static const win_t root_win = 1;

static std::deque<event_t> queue;

static win_t focus_win;		/* Toplevel with the keyboard focus. */
static win_t pointer_win;	/* Window under the pointer. */
static int pointer_x, pointer_y;	/* Root coordinates. */
static unsigned int button_state;	/* Button1Mask ... */

static win_t grab_win;		/* Pointer grab (explicit or implicit). */
static bool grab_explicit;
static bool grab_owner_events;
static long grab_mask;
static cursor_t grab_cursor;

static bool auto_repeat = true;
static win_t main_win;
static SDL_Cursor *current_cursor;

static char display_dummy, screen_dummy;


static obj *O(unsigned long h)
{
	if((h == 0) || (h >= objs.size()))
	    return(NULL);
	if((objs[h] == NULL) || (objs[h]->type == OBJ_FREE))
	    return(NULL);
	return(objs[h]);
}

static obj *W(win_t w)
{
	obj *o = O(w);
	return(((o != NULL) && (o->type == OBJ_WINDOW)) ? o : NULL);
}

static unsigned long NewObj(int type)
{
	obj *o = new obj();

	o->type = type;
	objs.push_back(o);
	return(objs.size() - 1);
}

/* Toplevel window that w is in, 0 if none (or w is root). */
static win_t Toplevel(win_t w)
{
	obj *o;

	while((o = W(w)) != NULL)
	{
	    if(o->parent == root_win)
		return(w);
	    w = o->parent;
	}
	return(0);
}

static bool IsDescendant(win_t ancestor, win_t w)
{
	obj *o;

	while((o = W(w)) != NULL)
	{
	    if(w == ancestor)
		return(true);
	    w = o->parent;
	}
	return(false);
}

/* True if w and all of its ancestors are mapped (and shown). */
static bool Viewable(win_t w)
{
	obj *o;

	if(w == root_win)
	    return(true);
	while((o = W(w)) != NULL)
	{
	    if(!o->mapped)
		return(false);
	    if(o->parent == root_win)
		return(!o->iconified);
	    w = o->parent;
	}
	return(false);
}

/* Position of w in root (screen) coordinates. */
static void RootPos(win_t w, int *x, int *y)
{
	obj *o;

	*x = 0;
	*y = 0;
	while(((o = W(w)) != NULL) && (w != root_win))
	{
	    *x += o->x;
	    *y += o->y;
	    w = o->parent;
	}
}

static void MarkDirty(win_t w)
{
	obj *o = W(Toplevel(w));

	if(o != NULL)
	    o->dirty = true;
}


/* ******************************************************************
 *
 *	Buffers.
 */
struct buffer {
	u_int32_t *data;
	int width, height;
};

/* The buffer of a drawable to draw on, marking its window dirty. */
static bool Target(drawable_t d, buffer *b)
{
	obj *o = O(d);

	if((o == NULL) || (o->data == NULL))
	    return(false);
	b->data = o->data;
	b->width = o->width;
	b->height = o->height;
	if(o->type == OBJ_WINDOW)
	    MarkDirty(d);
	return(true);
}

static inline void PutPixel(buffer *b, int x, int y, u_int32_t c)
{
	if((x >= 0) && (y >= 0) && (x < b->width) && (y < b->height))
	    b->data[(y * b->width) + x] = c;
}

static void FillRect(buffer *b, int x, int y, int w, int h, u_int32_t c)
{
	int i, j, x2 = x + w, y2 = y + h;

	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x2 > b->width) x2 = b->width;
	if(y2 > b->height) y2 = b->height;
	for(j = y; j < y2; j++)
	{
	    u_int32_t *p = &b->data[(j * b->width) + x];
	    for(i = x; i < x2; i++)
		*p++ = c;
	}
}

/*   Copies a w x h area at sx, sy of src (sw x sh, sbpl bytes per
 *   line) to tx, ty of b, clipped to both.
 */
static void Blit(
	buffer *b, int tx, int ty,
	const u_int32_t *src, int sw, int sh, int sbpl,
	int sx, int sy, int w, int h
)
{
	int j;

	if(sx < 0) { tx -= sx; w += sx; sx = 0; }
	if(sy < 0) { ty -= sy; h += sy; sy = 0; }
	if(tx < 0) { sx -= tx; w += tx; tx = 0; }
	if(ty < 0) { sy -= ty; h += ty; ty = 0; }
	if(sx + w > sw) w = sw - sx;
	if(sy + h > sh) h = sh - sy;
	if(tx + w > b->width) w = b->width - tx;
	if(ty + h > b->height) h = b->height - ty;
	if((w <= 0) || (h <= 0))
	    return;
	for(j = 0; j < h; j++)
	    memmove(
		&b->data[((ty + j) * b->width) + tx],
		(const u_int8_t *)src + ((sy + j) * sbpl) + (sx * 4),
		w * 4
	    );
}

/* Fills w with its background (pixel or tiled pixmap). */
static void ClearToBkg(win_t w)
{
	obj *o = W(w), *p;
	buffer b;
	int x, y;

	if((o == NULL) || (o->data == NULL))
	    return;
	b.data = o->data;
	b.width = o->width;
	b.height = o->height;
	p = O(o->bkg_pixmap);
	if((p != NULL) && (p->data != NULL))
	{
	    for(y = 0; y < o->height; y += p->height)
		for(x = 0; x < o->width; x += p->width)
		    Blit(&b, x, y, p->data, p->width, p->height,
			p->width * 4, 0, 0, p->width, p->height);
	}
	else
	{
	    FillRect(&b, 0, 0, o->width, o->height, (u_int32_t)o->bkg_pix);
	}
	MarkDirty(w);
}

/* Draws w and its mapped descendants onto b at x, y (clipped). */
static void Composite(win_t w, buffer *b, int x, int y, int cx, int cy, int cw, int ch)
{
	obj *o = W(w);
	size_t i;
	int x2, y2;

	if(o == NULL)
	    return;

	/* Clip to this window. */
	x2 = MIN(cx + cw, x + o->width);
	y2 = MIN(cy + ch, y + o->height);
	cx = MAX(cx, x);
	cy = MAX(cy, y);
	cw = x2 - cx;
	ch = y2 - cy;
	if((cw <= 0) || (ch <= 0))
	    return;

	if(o->data != NULL)
	    Blit(b, cx, cy, o->data, o->width, o->height, o->width * 4,
		cx - x, cy - y, cw, ch);

	for(i = 0; i < o->children.size(); i++)
	{
	    obj *c = W(o->children[i]);

	    if((c != NULL) && c->mapped)
		Composite(o->children[i], b, x + c->x, y + c->y, cx, cy, cw, ch);
	}
}

/* Shows the composited contents of toplevel w. */
static void Present(win_t w)
{
	obj *o = W(w);
	buffer b;

	if((o == NULL) || (o->sdl_win == NULL) || !o->mapped || o->iconified)
	    return;

	if((o->texture == NULL) ||
	   (o->tex_width != o->width) || (o->tex_height != o->height)
	)
	{
	    if(o->texture != NULL)
		SDL_DestroyTexture(o->texture);
	    o->texture = SDL_CreateTexture(
		o->renderer, SDL_PIXELFORMAT_RGB888,
		SDL_TEXTUREACCESS_STREAMING, o->width, o->height
	    );
	    o->tex_width = o->width;
	    o->tex_height = o->height;
	    free(o->comp);
	    o->comp = (u_int32_t *)calloc(o->width * o->height, 4);
	    SDL_RenderSetLogicalSize(o->renderer, o->width, o->height);
	}
	if((o->texture == NULL) || (o->comp == NULL))
	    return;

	b.data = o->comp;
	b.width = o->width;
	b.height = o->height;
	Composite(w, &b, 0, 0, 0, 0, o->width, o->height);

	SDL_UpdateTexture(o->texture, NULL, o->comp, o->width * 4);
	SDL_SetRenderDrawColor(o->renderer, 0, 0, 0, 255);
	SDL_RenderClear(o->renderer);
	SDL_RenderCopy(o->renderer, o->texture, NULL, NULL);
	SDL_RenderPresent(o->renderer);
	o->dirty = false;
}

static void PresentAll(void)
{
	obj *r = W(root_win);
	size_t i;

	if(r == NULL)
	    return;
	for(i = 0; i < r->children.size(); i++)
	{
	    obj *o = W(r->children[i]);

	    if((o != NULL) && o->dirty)
		Present(r->children[i]);
	}
}


/* ******************************************************************
 *
 *	Events.
 */

/* The event mask that selects events of type. */
static long TypeMask(int type)
{
	switch(type)
	{
	  case KeyPress: return(KeyPressMask);
	  case KeyRelease: return(KeyReleaseMask);
	  case ButtonPress: return(ButtonPressMask);
	  case ButtonRelease: return(ButtonReleaseMask);
	  case MotionNotify:
	    return(PointerMotionMask | ButtonMotionMask | Button1MotionMask |
		Button2MotionMask | Button3MotionMask | Button4MotionMask |
		Button5MotionMask);
	  case EnterNotify: return(EnterWindowMask);
	  case LeaveNotify: return(LeaveWindowMask);
	  case FocusIn:
	  case FocusOut: return(FocusChangeMask);
	  case Expose: return(ExposureMask);
	  case VisibilityNotify: return(VisibilityChangeMask);
	  case DestroyNotify:
	  case UnmapNotify:
	  case MapNotify:
	  case ConfigureNotify: return(StructureNotifyMask);
	  default: return(0);
	}
}

/* True if the motion event (with button state) is selected by mask. */
static bool MotionSelected(long mask, unsigned int state)
{
	if(mask & PointerMotionMask)
	    return(true);
	if(state & (Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask))
	{
	    if(mask & ButtonMotionMask)
		return(true);
	    if((state & Button1Mask) && (mask & Button1MotionMask)) return(true);
	    if((state & Button2Mask) && (mask & Button2MotionMask)) return(true);
	    if((state & Button3Mask) && (mask & Button3MotionMask)) return(true);
	}
	return(false);
}

static bool Selected(long mask, const event_t *ev)
{
	if(ev->type == MotionNotify)
	    return(MotionSelected(mask, ev->xmotion.state));
	return((mask & TypeMask(ev->type)) != 0);
}

static void Queue(const event_t *ev)
{
	queue.push_back(*ev);
}

/* Sends a non pointer event to w if w selected it. */
static void SendTo(win_t w, event_t *ev)
{
	obj *o = W(w);

	if(o == NULL)
	    return;
	ev->xany.window = w;
	ev->xany.display = osw_gui[0].display;
	if(Selected(o->event_mask, ev))
	    Queue(ev);
}

static unsigned int ModState(void)
{
	SDL_Keymod m = SDL_GetModState();
	unsigned int s = button_state;

	if(m & KMOD_SHIFT) s |= ShiftMask;
	if(m & KMOD_CAPS) s |= LockMask;
	if(m & KMOD_CTRL) s |= ControlMask;
	if(m & KMOD_ALT) s |= Mod1Mask;
	return(s);
}

/*   Fills in the window and coordinates of a device (key, button,
 *   motion or crossing) event for window w and queues it.
 */
static void DeliverDevice(win_t w, event_t *ev)
{
	int wx, wy;

	RootPos(w, &wx, &wy);
	ev->xany.window = w;
	ev->xany.display = osw_gui[0].display;
	/* The members x, y, x_root and y_root are at the same place in
	 * key, button, motion and crossing events.
	 */
	ev->xbutton.root = root_win;
	ev->xbutton.x_root = pointer_x;
	ev->xbutton.y_root = pointer_y;
	ev->xbutton.x = pointer_x - wx;
	ev->xbutton.y = pointer_y - wy;
	ev->xbutton.same_screen = True;
	ev->xbutton.time = SDL_GetTicks();
	Queue(ev);
}

/*   Delivers a device event starting at window w, propagating to the
 *   parents until a window selected it. Returns the window it went
 *   to or 0.
 */
static win_t Propagate(win_t w, event_t *ev)
{
	obj *o;

	while(((o = W(w)) != NULL) && (w != root_win))
	{
	    if(Selected(o->event_mask, ev))
	    {
		DeliverDevice(w, ev);
		return(w);
	    }
	    w = o->parent;
	}
	return(0);
}

/* Delivers a pointer event, taking a pointer grab into account. */
static win_t DeliverPointer(event_t *ev)
{
	if(W(grab_win) != NULL)
	{
	    if(grab_owner_events && (pointer_win != 0))
	    {
		win_t w = Propagate(pointer_win, ev);
		if(w != 0)
		    return(w);
	    }
	    if((ev->type == MotionNotify) ?
		MotionSelected(grab_mask, ev->xmotion.state) :
		((grab_mask & TypeMask(ev->type)) != 0)
	    )
	    {
		DeliverDevice(grab_win, ev);
		return(grab_win);
	    }
	    return(0);
	}
	if(pointer_win != 0)
	    return(Propagate(pointer_win, ev));
	return(0);
}

/* The innermost viewable window of toplevel top at root x, y. */
static win_t WindowAt(win_t w, int x, int y)
{
	obj *o = W(w);
	int i;

	if(o == NULL)
	    return(0);
	for(i = (int)o->children.size() - 1; i >= 0; i--)
	{
	    obj *c = W(o->children[i]);

	    if((c != NULL) && c->mapped &&
	       (x >= c->x) && (y >= c->y) &&
	       (x < c->x + c->width) && (y < c->y + c->height)
	    )
		return(WindowAt(o->children[i], x - c->x, y - c->y));
	}
	return(w);
}

static void UpdateCursor(void);

/* Moves the pointer to the window under root x, y in toplevel top. */
static void SetPointer(win_t top, int x, int y)
{
	obj *t = W(top);
	win_t w = 0;
	event_t ev;

	pointer_x = x;
	pointer_y = y;
	if((t != NULL) && Viewable(top) &&
	   (x >= t->x) && (y >= t->y) &&
	   (x < t->x + t->width) && (y < t->y + t->height)
	)
	    w = WindowAt(top, x - t->x, y - t->y);

	if(w != pointer_win)
	{
	    if(W(pointer_win) != NULL)
	    {
		memset(&ev, 0x00, sizeof(event_t));
		ev.type = LeaveNotify;
		ev.xcrossing.state = ModState();
		if(Selected(W(pointer_win)->event_mask, &ev))
		    DeliverDevice(pointer_win, &ev);
	    }
	    pointer_win = w;
	    if(W(pointer_win) != NULL)
	    {
		memset(&ev, 0x00, sizeof(event_t));
		ev.type = EnterNotify;
		ev.xcrossing.state = ModState();
		if(Selected(W(pointer_win)->event_mask, &ev))
		    DeliverDevice(pointer_win, &ev);
	    }
	    UpdateCursor();
	}
}

static void ReleaseGrab(void)
{
	grab_win = 0;
	grab_explicit = false;
	grab_cursor = 0;
	SDL_CaptureMouse(SDL_FALSE);
	UpdateCursor();
}

/*   Sends Expose (and VisibilityNotify) to w and its viewable
 *   descendants, as when w becomes viewable.
 */
static void ExposeTree(win_t w)
{
	obj *o = W(w);
	event_t ev;
	size_t i;

	if((o == NULL) || !Viewable(w))
	    return;

	if(!o->input_only)
	{
	    memset(&ev, 0x00, sizeof(event_t));
	    ev.type = VisibilityNotify;
	    ev.xvisibility.state = VisibilityUnobscured;
	    SendTo(w, &ev);

	    memset(&ev, 0x00, sizeof(event_t));
	    ev.type = Expose;
	    ev.xexpose.width = o->width;
	    ev.xexpose.height = o->height;
	    SendTo(w, &ev);
	}

	for(i = 0; i < o->children.size(); i++)
	    ExposeTree(o->children[i]);
}

static void SendConfigure(win_t w)
{
	obj *o = W(w);
	event_t ev;

	if(o == NULL)
	    return;
	memset(&ev, 0x00, sizeof(event_t));
	ev.type = ConfigureNotify;
	ev.xconfigure.event = w;
	ev.xconfigure.x = o->x;
	ev.xconfigure.y = o->y;
	ev.xconfigure.width = o->width;
	ev.xconfigure.height = o->height;
	SendTo(w, &ev);
}

static void SendStructure(win_t w, int type)
{
	event_t ev;

	memset(&ev, 0x00, sizeof(event_t));
	ev.type = type;
	ev.xmap.event = w;
	SendTo(w, &ev);
}

/* Changes the size of w's buffer, keeping its contents. */
static void ResizeBuffer(win_t w, int width, int height)
{
	obj *o = W(w);
	u_int32_t *data;
	buffer b;

	if((o == NULL) || ((o->width == width) && (o->height == height)))
	    return;
	if((o->data != NULL) && (width > 0) && (height > 0))
	{
	    data = (u_int32_t *)calloc(width * height, 4);
	    if(data == NULL)
		return;
	    b.data = data;
	    b.width = width;
	    b.height = height;
	    FillRect(&b, 0, 0, width, height, (u_int32_t)o->bkg_pix);
	    Blit(&b, 0, 0, o->data, o->width, o->height, o->width * 4,
		0, 0, o->width, o->height);
	    free(o->data);
	    o->data = data;
	}
	o->width = width;
	o->height = height;
}

/* The toplevel with the SDL window id, 0 if none. */
static win_t ToplevelOfSDL(Uint32 id)
{
	obj *r = W(root_win);
	size_t i;

	if(r == NULL)
	    return(0);
	for(i = 0; i < r->children.size(); i++)
	{
	    obj *o = W(r->children[i]);

	    if((o != NULL) && (o->sdl_win != NULL) &&
	       (SDL_GetWindowID(o->sdl_win) == id)
	    )
		return(r->children[i]);
	}
	return(0);
}

static void KeyEvent(int type, const SDL_KeyboardEvent *ke, char text)
{
	event_t ev;
	unsigned int keycode = ScancodeToKeycode(ke->keysym.scancode);
	win_t top = ToplevelOfSDL(ke->windowID), src;

	if(keycode == 0)
	    return;
	if(top == 0)
	    top = focus_win;
	if(top == 0)
	    return;

	/* The window under the pointer if it is in the focus window. */
	src = ((pointer_win != 0) && IsDescendant(top, pointer_win)) ?
	    pointer_win : top;

	memset(&ev, 0x00, sizeof(event_t));
	ev.type = type;
	ev.xkey.keycode = keycode;
	ev.xkey.state = ModState();
	ev.xkey.text = text;
	Propagate(src, &ev);
}

static void ButtonEvent(int type, win_t top, int x, int y, unsigned int button)
{
	event_t ev;
	unsigned int mask = (button >= 1 && button <= 5) ?
	    (Button1Mask << (button - 1)) : 0;
	win_t w;
	obj *t = W(top);

	if(t != NULL)
	    SetPointer(top, t->x + x, t->y + y);

	memset(&ev, 0x00, sizeof(event_t));
	ev.type = type;
	ev.xbutton.button = button;
	ev.xbutton.state = ModState();

	if(type == ButtonPress)
	{
	    w = DeliverPointer(&ev);
	    /* Implicit grab of the window that got the press. */
	    if((W(grab_win) == NULL) && (w != 0))
	    {
		grab_win = w;
		grab_explicit = false;
		grab_owner_events = false;
		grab_mask = W(w)->event_mask;
		SDL_CaptureMouse(SDL_TRUE);
	    }
	    button_state |= mask;
	}
	else
	{
	    DeliverPointer(&ev);
	    button_state &= ~mask;
	    if((button_state == 0) && !grab_explicit && (grab_win != 0))
		ReleaseGrab();
	}
}

/* Translates an SDL event into X events. */
static void Translate(SDL_Event *se)
{
	event_t ev;
	win_t top;
	obj *t;

	switch(se->type)
	{
	  case SDL_KEYDOWN:
	  case SDL_KEYUP:
	    {
		char text = '\0';
		SDL_Event next;

		/*   The character typed comes as a following text input
		 *   event.
		 */
		if((se->type == SDL_KEYDOWN) &&
		   (SDL_PeepEvents(&next, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1) &&
		   (next.type == SDL_TEXTINPUT)
		)
		{
		    SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_TEXTINPUT, SDL_TEXTINPUT);
		    if(((unsigned char)next.text.text[0] >= 0x20) &&
		       ((unsigned char)next.text.text[0] < 0x7f) &&
		       (next.text.text[1] == '\0')
		    )
			text = next.text.text[0];
		}

		if(se->key.repeat)
		{
		    /*   X sends a release and a press for each auto
		     *   repeat, and nothing when it is off.
		     */
		    if(!auto_repeat)
			break;
		    KeyEvent(KeyRelease, &se->key, '\0');
		}
		KeyEvent((se->type == SDL_KEYDOWN) ? KeyPress : KeyRelease,
		    &se->key, text);
	    }
	    break;

	  case SDL_MOUSEMOTION:
	    top = ToplevelOfSDL(se->motion.windowID);
	    t = W(top);
	    if(t == NULL)
		break;
	    SetPointer(top, t->x + se->motion.x, t->y + se->motion.y);
	    memset(&ev, 0x00, sizeof(event_t));
	    ev.type = MotionNotify;
	    ev.xmotion.state = ModState();
	    DeliverPointer(&ev);
	    break;

	  case SDL_MOUSEBUTTONDOWN:
	  case SDL_MOUSEBUTTONUP:
	    {
		unsigned int button;

		switch(se->button.button)
		{
		  case SDL_BUTTON_LEFT: button = Button1; break;
		  case SDL_BUTTON_MIDDLE: button = Button2; break;
		  case SDL_BUTTON_RIGHT: button = Button3; break;
		  default: button = 0; break;
		}
		if(button == 0)
		    break;
		ButtonEvent(
		    (se->type == SDL_MOUSEBUTTONDOWN) ? ButtonPress : ButtonRelease,
		    ToplevelOfSDL(se->button.windowID),
		    se->button.x, se->button.y, button
		);
	    }
	    break;

	  case SDL_MOUSEWHEEL:
	    /* X reports the wheel as buttons 4 (up) and 5 (down). */
	    top = ToplevelOfSDL(se->wheel.windowID);
	    t = W(top);
	    if((t == NULL) || (se->wheel.y == 0))
		break;
	    {
		unsigned int button = (se->wheel.y > 0) ? Button4 : Button5;
		int x = pointer_x - t->x, y = pointer_y - t->y;

		ButtonEvent(ButtonPress, top, x, y, button);
		ButtonEvent(ButtonRelease, top, x, y, button);
	    }
	    break;

	  case SDL_WINDOWEVENT:
	    top = ToplevelOfSDL(se->window.windowID);
	    t = W(top);
	    if(t == NULL)
		break;
	    switch(se->window.event)
	    {
	      case SDL_WINDOWEVENT_CLOSE:
		memset(&ev, 0x00, sizeof(event_t));
		ev.type = ClientMessage;
		ev.xclient.window = top;
		ev.xclient.display = osw_gui[0].display;
		ev.xclient.message_type = osw_atom.wm_protocols;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = (long)osw_atom.wm_delete_window;
		Queue(&ev);
		break;

	      case SDL_WINDOWEVENT_FOCUS_GAINED:
		focus_win = top;
		memset(&ev, 0x00, sizeof(event_t));
		ev.type = FocusIn;
		SendTo(top, &ev);
		break;

	      case SDL_WINDOWEVENT_FOCUS_LOST:
		if(focus_win == top)
		    focus_win = 0;
		memset(&ev, 0x00, sizeof(event_t));
		ev.type = FocusOut;
		SendTo(top, &ev);
		break;

	      case SDL_WINDOWEVENT_MOVED:
		if((t->x != se->window.data1) || (t->y != se->window.data2))
		{
		    t->x = se->window.data1;
		    t->y = se->window.data2;
		    SendConfigure(top);
		}
		break;

	      case SDL_WINDOWEVENT_SIZE_CHANGED:
		if((t->width != se->window.data1) || (t->height != se->window.data2))
		{
		    ResizeBuffer(top, se->window.data1, se->window.data2);
		    SendConfigure(top);
		    ExposeTree(top);
		    t->dirty = true;
		}
		break;

	      case SDL_WINDOWEVENT_MINIMIZED:
		/* Iconified by the window manager: like X, unmapped. */
		if(!t->iconified)
		{
		    t->iconified = true;
		    SendStructure(top, UnmapNotify);
		}
		break;

	      case SDL_WINDOWEVENT_RESTORED:
	      case SDL_WINDOWEVENT_SHOWN:
		if(t->iconified)
		{
		    t->iconified = false;
		    SendStructure(top, MapNotify);
		    ExposeTree(top);
		}
		t->dirty = true;
		break;

	      case SDL_WINDOWEVENT_EXPOSED:
		t->dirty = true;
		break;

	      case SDL_WINDOWEVENT_LEAVE:
		SetPointer(0, pointer_x, pointer_y);
		break;
	    }
	    break;

	  default:
	    break;
	}
}

/* Reads and translates all pending SDL events. */
static void Pump(void)
{
	SDL_Event se;

	if(!IDC())
	    return;
	while(SDL_PollEvent(&se))
	    Translate(&se);
}

/* Waits until there is at least one event in the queue. */
static void WaitForEvent(void)
{
	SDL_Event se;

	Pump();
	while(queue.empty() && IDC())
	{
	    PresentAll();
	    if(SDL_WaitEventTimeout(&se, 100))
		Translate(&se);
	    Pump();
	}
}

/* Updates the modifier key states, as osw-x.cpp does. */
static void ManageEvent(event_t *event)
{
	osw_gui_struct *gui = &osw_gui[0];
	keycode_t keycode;

	switch(event->type)
	{
	  case KeyPress:
	    keycode = event->xkey.keycode;
	    if((keycode == osw_keycode.alt_left) ||
	       (keycode == osw_keycode.alt_right)
	    )
		gui->alt_key_state = True;
	    else if((keycode == osw_keycode.ctrl_left) ||
		    (keycode == osw_keycode.ctrl_right)
	    )
		gui->ctrl_key_state = True;
	    else if((keycode == osw_keycode.shift_left) ||
		    (keycode == osw_keycode.shift_right)
	    )
		gui->shift_key_state = True;
	    else if(keycode == osw_keycode.caps_lock)
		gui->caps_lock_on = !gui->caps_lock_on;
	    else if(keycode == osw_keycode.num_lock)
		gui->num_lock_on = !gui->num_lock_on;
	    else if(keycode == osw_keycode.scroll_lock)
		gui->scroll_lock_on = !gui->scroll_lock_on;
	    break;

	  case KeyRelease:
	    keycode = event->xkey.keycode;
	    if((keycode == osw_keycode.alt_left) ||
	       (keycode == osw_keycode.alt_right)
	    )
		gui->alt_key_state = False;
	    else if((keycode == osw_keycode.ctrl_left) ||
		    (keycode == osw_keycode.ctrl_right)
	    )
		gui->ctrl_key_state = False;
	    else if((keycode == osw_keycode.shift_left) ||
		    (keycode == osw_keycode.shift_right)
	    )
		gui->shift_key_state = False;
	    break;
	}
}

}	/* namespace static_osw_sdl */

using namespace static_osw_sdl;


/* ******************************************************************
 *
 *	GUI init.
 */

/* Parses an X geometry string [=][WxH][{+-}X{+-}Y]. */
static void ParseGeometry(
	const char *s, int *x, int *y, unsigned int *w, unsigned int *h
)
{
	int a, b;
	char sx, sy;

	if(*s == '=')
	    s++;
	if(sscanf(s, "%dx%d", &a, &b) == 2)
	{
	    *w = a;
	    *h = b;
	    while((*s != '\0') && (*s != '+') && (*s != '-'))
		s++;
	}
	if(sscanf(s, "%c%d%c%d", &sx, &a, &sy, &b) == 4)
	{
	    *x = (sx == '-') ? -a : a;
	    *y = (sy == '-') ? -b : b;
	}
}

int OSWGUIConnect(int argc, char *argv[])
{
	osw_gui_struct *gui = &osw_gui[0];
	SDL_Rect bounds;
	int i;
	u_int8_t r, g, b;
	char *fg_pix_str = NULL;
	char *bg_pix_str = NULL;
	char *def_font_name = NULL;
	obj *root;


	memset(gui, 0x00, sizeof(osw_gui_struct));

	/* Parse arguments (those of osw-x.cpp that apply). */
	for(i = 0; i < argc; i++)
	{
	    if(argv[i] == NULL)
		continue;

	    if(!strcmp(argv[i], "--geometry") ||
	       !strcmp(argv[i], "-geometry")
	    )
	    {
		if(++i < argc)
		{
		    ParseGeometry(argv[i],
			&gui->def_toplevel_x, &gui->def_toplevel_y,
			&gui->def_toplevel_width, &gui->def_toplevel_height
		    );
		    gui->def_geometry_set = True;
		}
	    }
	    else if(!strcmp(argv[i], "--font") ||
		    !strcmp(argv[i], "-font") ||
		    !strcmp(argv[i], "--fn") ||
		    !strcmp(argv[i], "-fn")
	    )
	    {
		if(++i < argc)
		    def_font_name = argv[i];
	    }
	    else if(!strcmp(argv[i], "--background") ||
		    !strcmp(argv[i], "-background") ||
		    !strcmp(argv[i], "--bg") ||
		    !strcmp(argv[i], "-bg")
	    )
	    {
		if(++i < argc)
		    bg_pix_str = argv[i];
	    }
	    else if(!strcmp(argv[i], "--foreground") ||
		    !strcmp(argv[i], "-foreground") ||
		    !strcmp(argv[i], "--fg") ||
		    !strcmp(argv[i], "-fg")
	    )
	    {
		if(++i < argc)
		    fg_pix_str = argv[i];
	    }
	    else if(!strcmp(argv[i], "--display") ||
		    !strcmp(argv[i], "--dpy") ||
		    !strcmp(argv[i], "-display") ||
		    !strcmp(argv[i], "-dpy") ||
		    !strcmp(argv[i], "--visual") ||
		    !strcmp(argv[i], "-visual") ||
		    !strcmp(argv[i], "--depth") ||
		    !strcmp(argv[i], "-depth")
	    )
	    {
		i++;	/* Only for X. */
	    }
	}

	/*   The program handles SIGINT and SIGTERM itself (the sound
	 *   and joystick code sets this too).
	 */
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
	/* Scale the windows up by whole pixels (Retina displays). */
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
	if(SDL_InitSubSystem(SDL_INIT_VIDEO))
	{
	    fprintf(stderr, "Cannot initialize SDL video: %s\n",
		SDL_GetError()
	    );
	    return(1);
	}

	gui->display = (display_t *)&display_dummy;
	gui->scr_ptr = (screen_ptr_t *)&screen_dummy;
	gui->visual = NULL;
	gui->depth = 24;
	gui->actual_depth = 32;
	gui->z_bytes = 4;
	gui->red_mask = 0xff0000;
	gui->green_mask = 0x00ff00;
	gui->blue_mask = 0x0000ff;
	gui->black_pix = 0x000000;
	gui->white_pix = 0xffffff;
	gui->def_use_shm = False;
	gui->def_sync_shm = False;

	/* Root window, the size of the main display. */
	objs.clear();
	objs.push_back(NULL);			/* Handle 0 is None. */
	NewObj(OBJ_WINDOW);			/* Handle 1 is root. */
	root = W(root_win);
	if(SDL_GetDisplayBounds(0, &bounds))
	{
	    bounds.w = 1024;
	    bounds.h = 768;
	}
	root->width = bounds.w;
	root->height = bounds.h;
	root->mapped = true;
	gui->root_win = root_win;
	gui->display_width = bounds.w;
	gui->display_height = bounds.h;

	gui->std_cursor = OSWLoadBasicCursor(XC_left_ptr);

	StringParseStdColor(
	    (bg_pix_str == NULL) ? "#000000" : bg_pix_str, &r, &g, &b
	);
	OSWLoadPixelRGB(&gui->def_bg_pix, r, g, b);
	StringParseStdColor(
	    (fg_pix_str == NULL) ? "#ffffff" : fg_pix_str, &r, &g, &b
	);
	OSWLoadPixelRGB(&gui->def_fg_pix, r, g, b);

	/* Standard, bold and default fonts. */
	OSWLoadFont(&gui->std_font, "7x14");
	OSWLoadFont(&gui->bold_font, "7x14");
	OSWLoadFont(
	    &gui->def_font,
	    (def_font_name == NULL) ? "7x14" : def_font_name
	);

	OSWSetFgPix(gui->white_pix);
	OSWSetFont(gui->std_font);

	OSWLoadKeyCodes();

	/* Atoms only need to be distinct. */
	osw_atom._motif_wm_all_clients = 1;
	osw_atom._motif_wm_hints = 2;
	osw_atom._motif_wm_info = 3;
	osw_atom._motif_wm_menu = 4;
	osw_atom._motif_wm_messages = 5;
	osw_atom._motif_wm_offset = 6;
	osw_atom._motif_wm_query = 7;
	osw_atom.wm_delete_window = 8;
	osw_atom.wm_protocols = 9;
	osw_atom.wm_save_yourself = 10;
	osw_atom.wm_state = 11;
	osw_atom.wm_take_focus = 12;
	osw_atom._xrootpmap_id = 13;
	osw_atom._xrootcolor_pixel = 14;
	osw_atom.enl_msg = 15;
	osw_atom.enlightenment_desktop = 16;
	osw_atom.enlightenment_comms = 17;

	SDL_StartTextInput();

	return(0);
}

void OSWGUIDisconnect(void)
{
	osw_gui_struct *gui = &osw_gui[0];
	size_t i;

	if(!IDC())
	    return;

	OSWUnloadFont(&gui->std_font);
	OSWUnloadFont(&gui->bold_font);
	OSWUnloadFont(&gui->def_font);

	for(i = 0; i < objs.size(); i++)
	{
	    obj *o = objs[i];

	    if(o == NULL)
		continue;
	    if(o->texture != NULL)
		SDL_DestroyTexture(o->texture);
	    if(o->renderer != NULL)
		SDL_DestroyRenderer(o->renderer);
	    if(o->sdl_win != NULL)
		SDL_DestroyWindow(o->sdl_win);
	    free(o->data);
	    free(o->comp);
	    delete o;
	}
	objs.clear();
	SDL_SetCursor(SDL_GetDefaultCursor());
	for(i = 0; i < cursors.size(); i++)
	{
	    if(cursors[i] != NULL)
		SDL_FreeCursor(cursors[i]);
	}
	cursors.clear();
	queue.clear();
	focus_win = pointer_win = grab_win = 0;

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	gui->display = NULL;
}

int OSWLoadKeyCodes(void)
{
	osw_keycode_struct *k = &osw_keycode;

	/* The same keysyms as osw-x.cpp. */
	k->esc = SymToKeycode("Escape");
	k->f1 = SymToKeycode("F1");
	k->f2 = SymToKeycode("F2");
	k->f3 = SymToKeycode("F3");
	k->f4 = SymToKeycode("F4");
	k->f5 = SymToKeycode("F5");
	k->f6 = SymToKeycode("F6");
	k->f7 = SymToKeycode("F7");
	k->f8 = SymToKeycode("F8");
	k->f9 = SymToKeycode("F9");
	k->f10 = SymToKeycode("F10");
	k->f11 = SymToKeycode("F11");
	k->f12 = SymToKeycode("F12");
	k->f13 = k->f14 = k->f15 = k->f16 = k->f17 = k->f18 = 0;
	k->f19 = k->f20 = k->f21 = k->f22 = k->f23 = k->f24 = 0;
	k->f25 = k->f26 = k->f27 = k->f28 = k->f29 = k->f30 = 0;
	k->f31 = k->f32 = k->f33 = k->f34 = k->f35 = 0;

	k->tilde = SymToKeycode("asciitilde");
	k->num_1 = SymToKeycode("1");
	k->num_2 = SymToKeycode("2");
	k->num_3 = SymToKeycode("3");
	k->num_4 = SymToKeycode("4");
	k->num_5 = SymToKeycode("5");
	k->num_6 = SymToKeycode("6");
	k->num_7 = SymToKeycode("7");
	k->num_8 = SymToKeycode("8");
	k->num_9 = SymToKeycode("9");
	k->num_0 = SymToKeycode("0");
	k->colon = SymToKeycode("colon");
	k->lessthan = SymToKeycode("less");
	k->greaterthan = SymToKeycode("greater");
	k->questionmark = SymToKeycode("question");
	k->at = SymToKeycode("at");
	k->underscore = SymToKeycode("underscore");
	k->braketleft = SymToKeycode("bracketleft");
	k->braketright = SymToKeycode("bracketright");
	k->quoteleft = SymToKeycode("grave");
	k->exclamation = SymToKeycode("exclam");
	k->numbersign = SymToKeycode("numbersign");
	k->dollarsign = SymToKeycode("dollar");
	k->percent = SymToKeycode("percent");
	k->ampersand = SymToKeycode("ampersand");
	k->apostrophe = SymToKeycode("apostrophe");
	k->parenleft = SymToKeycode("parenleft");
	k->parenright = SymToKeycode("parenright");
	k->asterisk = SymToKeycode("asterisk");
	k->plus = SymToKeycode("plus");
	k->minus = SymToKeycode("minus");
	k->equal = SymToKeycode("equal");
	k->brace_left = SymToKeycode("braceleft");
	k->brace_right = SymToKeycode("braceright");
	k->bar = SymToKeycode("bar");
	k->backslash = SymToKeycode("backslash");
	k->backspace = SymToKeycode("BackSpace");

	k->tab = SymToKeycode("Tab");
	k->alpha_q = k->alpha_Q = SymToKeycode("q");
	k->alpha_w = k->alpha_W = SymToKeycode("w");
	k->alpha_e = k->alpha_E = SymToKeycode("e");
	k->alpha_r = k->alpha_R = SymToKeycode("r");
	k->alpha_t = k->alpha_T = SymToKeycode("t");
	k->alpha_y = k->alpha_Y = SymToKeycode("y");
	k->alpha_u = k->alpha_U = SymToKeycode("u");
	k->alpha_i = k->alpha_I = SymToKeycode("i");
	k->alpha_o = k->alpha_O = SymToKeycode("o");
	k->alpha_p = k->alpha_P = SymToKeycode("p");
	k->enter = SymToKeycode("Return");

	k->caps_lock = SymToKeycode("Caps_Lock");
	k->alpha_a = k->alpha_A = SymToKeycode("a");
	k->alpha_s = k->alpha_S = SymToKeycode("s");
	k->alpha_d = k->alpha_D = SymToKeycode("d");
	k->alpha_f = k->alpha_F = SymToKeycode("f");
	k->alpha_g = k->alpha_G = SymToKeycode("g");
	k->alpha_h = k->alpha_H = SymToKeycode("h");
	k->alpha_j = k->alpha_J = SymToKeycode("j");
	k->alpha_k = k->alpha_K = SymToKeycode("k");
	k->alpha_l = k->alpha_L = SymToKeycode("l");
	k->semicolon = SymToKeycode("semicolon");
	k->quote = SymToKeycode("apostrophe");

	k->shift_left = SymToKeycode("Shift_L");
	k->alpha_z = k->alpha_Z = SymToKeycode("z");
	k->alpha_x = k->alpha_X = SymToKeycode("x");
	k->alpha_c = k->alpha_C = SymToKeycode("c");
	k->alpha_v = k->alpha_V = SymToKeycode("v");
	k->alpha_b = k->alpha_B = SymToKeycode("b");
	k->alpha_n = k->alpha_N = SymToKeycode("n");
	k->alpha_m = k->alpha_M = SymToKeycode("m");
	k->comma = SymToKeycode("comma");
	k->period = SymToKeycode("period");
	k->slash = SymToKeycode("slash");
	k->shift_right = SymToKeycode("Shift_R");

	k->ctrl_left = SymToKeycode("Control_L");
	k->alt_left = SymToKeycode("Alt_L");
	k->win95_start = 0;
	k->space = SymToKeycode("space");
	k->alt_right = SymToKeycode("Alt_R");
	k->ctrl_right = SymToKeycode("Control_R");

	k->print_screen = SymToKeycode("Sys_Req");
	k->scroll_lock = SymToKeycode("Scroll_Lock");
	k->pause = SymToKeycode("Pause");
	k->insert = SymToKeycode("Insert");
	k->home = SymToKeycode("Home");
	k->page_up = SymToKeycode("Prior");
	k->ddelete = SymToKeycode("Delete");
	k->end = SymToKeycode("End");
	k->page_down = SymToKeycode("Next");

	k->cursor_up = SymToKeycode("Up");
	k->cursor_right = SymToKeycode("Right");
	k->cursor_down = SymToKeycode("Down");
	k->cursor_left = SymToKeycode("Left");

	k->num_lock = SymToKeycode("Num_Lock");
	k->np_slash = SymToKeycode("KP_Divide");
	k->np_asterisk = SymToKeycode("KP_Multiply");
	k->np_minus = SymToKeycode("KP_Subtract");
	k->np_add = SymToKeycode("KP_Add");
	k->np_enter = SymToKeycode("KP_Enter");
	k->np_1 = SymToKeycode("KP_1");
	k->np_2 = SymToKeycode("KP_2");
	k->np_3 = SymToKeycode("KP_3");
	k->np_4 = SymToKeycode("KP_4");
	k->np_5 = SymToKeycode("KP_5");
	k->np_6 = SymToKeycode("KP_6");
	k->np_7 = SymToKeycode("KP_7");
	k->np_8 = SymToKeycode("KP_8");
	k->np_9 = SymToKeycode("KP_9");
	k->np_0 = SymToKeycode("KP_0");
	k->np_period = SymToKeycode("KP_Decimal");

	return(0);
}


/* ******************************************************************
 *
 *	Keyboard.
 */
char OSWGetASCIIFromKeyCode(
	key_event_t *ke,
	bool_t shift,
	bool_t alt,
	bool_t ctrl
)
{
	const char *sym;
	size_t i;

	if(!IDC() || (ke == NULL))
	    return('\0');

	/* The character typed, if SDL gave one. */
	if(ke->text != '\0')
	    return(ke->text);

	/*   As osw-x.cpp: the keysym of the keycode (shifted by the
	 *   shift key state), then the character of its name.
	 */
	sym = KeycodeSym(ke->keycode, osw_gui[0].shift_key_state ? 1 : 0);
	if((sym == NULL) && osw_gui[0].shift_key_state)
	    sym = KeycodeSym(ke->keycode, 0);
	if(sym == NULL)
	    return('\0');

	if((sym[1] == '\0') && (((sym[0] >= '0') && (sym[0] <= '9')) ||
	   ((sym[0] >= 'a') && (sym[0] <= 'z')) ||
	   ((sym[0] >= 'A') && (sym[0] <= 'Z')))
	)
	    return(sym[0]);
	for(i = 0; i < sizeof(keychar) / sizeof(keychar[0]); i++)
	{
	    if(!strcmp(keychar[i].name, sym))
		return(keychar[i].c);
	}
	return('\0');
}

const char *OSWGetKeyCodeName(keycode_t keycode)
{
	static char rtn_str[128];
	const char *sym;
	size_t i;

	if(keycode == 0)
	    return("#0");

	sym = KeycodeSym(keycode, 0);
	if(sym != NULL)
	{
	    if((sym[1] == '\0') && (((sym[0] >= '0') && (sym[0] <= '9')) ||
	       ((sym[0] >= 'a') && (sym[0] <= 'z')))
	    )
		return(sym);
	    for(i = 0; i < sizeof(keyname) / sizeof(keyname[0]); i++)
	    {
		if(!strcmp(keyname[i].sym, sym))
		    return(keyname[i].name);
	    }
	}
	sprintf(rtn_str, "#%u", keycode);
	return(rtn_str);
}

int OSWIsModifierKey(keycode_t keycode)
{
	if((keycode == osw_keycode.alt_left) ||
	   (keycode == osw_keycode.alt_right) ||
	   (keycode == osw_keycode.ctrl_left) ||
	   (keycode == osw_keycode.ctrl_right) ||
	   (keycode == osw_keycode.shift_left) ||
	   (keycode == osw_keycode.shift_right)
	)
	    return(1);
	else
	    return(0);
}

void OSWKBAutoRepeatOff(void)
{
	auto_repeat = false;
}

void OSWKBAutoRepeatOn(void)
{
	auto_repeat = true;
}


/* ******************************************************************
 *
 *	GUI systems and runtime.
 */
void OSWGUISync(bool_t discard)
{
	if(!IDC())
	    return;
	Pump();
	PresentAll();
	if(discard)
	    queue.clear();
}

void OSWGetPointerCoords(
	win_t w,
	int *root_x, int *root_y,
	int *wx, int *wy
)
{
	int x, y, gx = 0, gy = 0;

	if(root_x != NULL) *root_x = 0;
	if(root_y != NULL) *root_y = 0;
	if(wx != NULL) *wx = 0;
	if(wy != NULL) *wy = 0;
	if(!IDC() || (W(w) == NULL))
	    return;

	SDL_GetGlobalMouseState(&gx, &gy);
	RootPos(w, &x, &y);
	if(root_x != NULL) *root_x = gx;
	if(root_y != NULL) *root_y = gy;
	if(wx != NULL) *wx = gx - x;
	if(wy != NULL) *wy = gy - y;
}

int OSWGrabPointer(
	win_t grab_w,
	bool_t events_rel_grab_w,
	eventmask_t eventmask,
	int pointer_mode,
	int keyboard_mode,
	win_t confine_w,
	cursor_t cursor
)
{
	if(!IDC() || !Viewable(grab_w))
	    return(!GrabSuccess);

	grab_win = grab_w;
	grab_explicit = true;
	grab_owner_events = !events_rel_grab_w;
	grab_mask = eventmask;
	grab_cursor = cursor;
	SDL_CaptureMouse(SDL_TRUE);
	UpdateCursor();
	return(GrabSuccess);
}

void OSWUngrabPointer(void)
{
	if(!IDC())
	    return;
	ReleaseGrab();
}

void OSWGUIFree(void **ptr)
{
	if((ptr == NULL) || (*ptr == NULL))
	    return;
	free(*ptr);
	*ptr = NULL;
}

void *OSWFetchDDE(int *bytes)
{
	char *text, *buf;

	if(!IDC() || (bytes == NULL))
	    return(NULL);
	*bytes = 0;
	text = SDL_GetClipboardText();
	if(text == NULL)
	    return(NULL);
	if(*text == '\0')
	{
	    SDL_free(text);
	    return(NULL);
	}
	*bytes = strlen(text);
	buf = (char *)malloc(*bytes);
	if(buf != NULL)
	    memcpy(buf, text, *bytes);
	else
	    *bytes = 0;
	SDL_free(text);
	return(buf);
}

void OSWPutDDE(void *buf, int bytes)
{
	char *text;

	if(!IDC() || (buf == NULL) || (bytes <= 0))
	    return;
	text = (char *)malloc(bytes + 1);
	if(text == NULL)
	    return;
	memcpy(text, buf, bytes);
	text[bytes] = '\0';
	SDL_SetClipboardText(text);
	free(text);
}

visual_t *OSWGetVisualByCriteria(
	visual_info_mask_t vinfo_mask,
	visual_info_t criteria_vinfo
)
{
	return(NULL);
}

visual_t *OSWGetVisualByID(visual_id_t vid)
{
	return(NULL);
}


/* ******************************************************************
 *
 *	Fonts.
 */
int OSWLoadFont(font_t **font, const char *fontname)
{
	const OSWSDLFont *f = NULL;
	size_t i;

	if(!IDC() || (fontname == NULL) || (font == NULL))
	    return(-1);

	for(i = 0; i < sizeof(osw_sdl_fonts) / sizeof(osw_sdl_fonts[0]); i++)
	{
	    if(!strcmp(osw_sdl_fonts[i].name, fontname))
		f = &osw_sdl_fonts[i];
	}
	if(f == NULL)
	{
	    fprintf(stderr,
 "OSWLoadFont(): %s: Not a built in font, using 7x14.\n",
		fontname
	    );
	    f = &osw_sdl_fonts[0];
	}

	*font = (font_t *)calloc(1, sizeof(font_t));
	if(*font == NULL)
	    return(-1);
	(*font)->char_width = 0;
	(*font)->char_height = 0;
	(*font)->total_chars = 256;
	(*font)->actual = f;
	return(0);
}

font_t *OSWQueryCurrentFont(void)
{
	return(osw_gui[0].current_font);
}

void OSWSetFont(font_t *font)
{
	if(!IDC() || (font == NULL) || (font->actual == NULL))
	    return;
	osw_gui[0].current_font = font;
}

void OSWUnloadFont(font_t **font)
{
	if((font == NULL) || (*font == NULL))
	    return;
	if(*font == osw_gui[0].current_font)
	    osw_gui[0].current_font = NULL;
	free(*font);
	*font = NULL;
}


/* ******************************************************************
 *
 *	Colors.
 */
int OSWLoadPixelRGB(pixel_t *pix_rtn, u_int8_t r, u_int8_t g, u_int8_t b)
{
	if(pix_rtn == NULL)
	    return(-1);
	if(!IDC())
	{
	    *pix_rtn = 0;
	    return(-1);
	}
	*pix_rtn = ((pixel_t)r << 16) | ((pixel_t)g << 8) | (pixel_t)b;
	return(0);
}

int OSWLoadPixelHSL(pixel_t *pix_rtn, u_int8_t h, u_int8_t s, u_int8_t l)
{
	return(-1);
}

/*   X converts "rgbi:" intensities to RGB with Xcms's intensity
 *   tables, which are not linear and differ for red, green and blue
 *   (0.4 gives 177, 170 and 165). For each of them, the smallest
 *   intensity (in 1/100000) that X maps to each 8 bit level 1 to
 *   255, measured with XParseColor() on Xvfb.
 */
static const int rgbi_threshold[3][255] = {
	{	/* Red. */
	    1, 1, 1, 1, 1, 1, 1, 1, 1, 47,
	    100, 110, 121, 132, 143, 157, 174, 191, 208, 225,
	    245, 269, 293, 317, 341, 369, 401, 433, 465, 497,
	    534, 575, 615, 656, 697, 743, 793, 844, 895, 945,
	    1001, 1062, 1124, 1185, 1246, 1313, 1386, 1458, 1531, 1604,
	    1682, 1767, 1852, 1937, 2022, 2113, 2211, 2309, 2407, 2505,
	    2610, 2722, 2833, 2945, 3057, 3176, 3302, 3428, 3554, 3681,
	    3815, 3956, 4098, 4239, 4380, 4530, 4687, 4845, 5002, 5159,
	    5325, 5499, 5673, 5847, 6021, 6203, 6394, 6586, 6777, 6968,
	    7168, 7377, 7586, 7795, 8003, 8222, 8449, 8676, 8903, 9131,
	    9367, 9614, 9860, 10106, 10352, 10608, 10874, 11140, 11405, 11671,
	    11946, 12232, 12518, 12803, 13089, 13385, 13691, 13997, 14303, 14609,
	    14926, 15253, 15580, 15906, 16233, 16571, 16919, 17268, 17616, 17964,
	    18323, 18693, 19063, 19433, 19803, 20184, 20576, 20969, 21361, 21753,
	    22156, 22571, 22985, 23400, 23814, 24240, 24678, 25115, 25552, 25990,
	    26438, 26899, 27359, 27820, 28280, 28752, 29236, 29720, 30204, 30687,
	    31183, 31690, 32198, 32705, 33212, 33732, 34263, 34794, 35326, 35857,
	    36400, 36955, 37511, 38066, 38621, 39188, 39768, 40347, 40927, 41506,
	    42098, 42702, 43306, 43910, 44514, 45130, 45758, 46386, 47015, 47643,
	    48284, 48937, 49590, 50243, 50896, 51561, 52238, 52916, 53594, 54271,
	    54961, 55664, 56366, 57068, 57771, 58485, 59212, 59939, 60667, 61394,
	    62133, 62885, 63637, 64389, 65140, 65905, 66681, 67458, 68234, 69011,
	    69800, 70601, 71402, 72203, 73005, 73818, 74644, 75470, 76296, 77121,
	    77960, 78810, 79660, 80510, 81361, 82223, 83098, 83973, 84847, 85722,
	    86609, 87508, 88407, 89305, 90204, 91115, 92038, 92961, 93884, 94807,
	    95742, 96689, 97635, 98582, 99529
	},
	{	/* Green. */
	    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	    1, 1, 1, 1, 1, 1, 1, 1, 1, 42,
	    95, 119, 142, 165, 189, 217, 249, 281, 313, 346,
	    383, 426, 468, 510, 553, 601, 655, 709, 763, 816,
	    877, 943, 1009, 1076, 1142, 1215, 1295, 1375, 1455, 1535,
	    1622, 1716, 1811, 1905, 2000, 2102, 2212, 2322, 2432, 2542,
	    2660, 2786, 2912, 3039, 3165, 3300, 3443, 3587, 3730, 3874,
	    4026, 4188, 4349, 4511, 4672, 4843, 5023, 5203, 5383, 5563,
	    5753, 5952, 6151, 6350, 6550, 6759, 6978, 7197, 7416, 7635,
	    7865, 8104, 8343, 8583, 8822, 9072, 9332, 9592, 9853, 10113,
	    10384, 10665, 10946, 11228, 11509, 11801, 12104, 12407, 12709, 13012,
	    13326, 13650, 13975, 14300, 14624, 14960, 15306, 15653, 15999, 16346,
	    16703, 17072, 17440, 17809, 18177, 18557, 18948, 19338, 19729, 20120,
	    20522, 20935, 21347, 21760, 22173, 22597, 23032, 23467, 23902, 24337,
	    24783, 25240, 25697, 26154, 26611, 27079, 27558, 28037, 28516, 28995,
	    29484, 29985, 30485, 30986, 31486, 31998, 32519, 33041, 33563, 34085,
	    34617, 35160, 35703, 36245, 36788, 37341, 37905, 38468, 39031, 39595,
	    40168, 40751, 41335, 41918, 42502, 43095, 43698, 44301, 44904, 45507,
	    46119, 46741, 47363, 47985, 48607, 49238, 49879, 50519, 51159, 51799,
	    52448, 53106, 53764, 54422, 55080, 55746, 56420, 57095, 57770, 58444,
	    59127, 59817, 60508, 61198, 61889, 62587, 63293, 63998, 64704, 65409,
	    66122, 66842, 67562, 68281, 69001, 69727, 70460, 71193, 71925, 72658,
	    73397, 74141, 74886, 75631, 76376, 77126, 77881, 78637, 79392, 80148,
	    80908, 81673, 82438, 83203, 83969, 84738, 85511, 86285, 87058, 87832,
	    88609, 89389, 90170, 90950, 91730, 92514, 93300, 94086, 94872, 95658,
	    96446, 97236, 98027, 98817, 99607
	},
	{	/* Blue. */
	    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	    1, 1, 1, 1, 68, 142, 157, 172, 186, 201,
	    220, 242, 264, 286, 308, 335, 366, 396, 427, 458,
	    494, 535, 575, 616, 657, 703, 755, 807, 859, 910,
	    968, 1032, 1096, 1160, 1225, 1295, 1373, 1450, 1527, 1604,
	    1689, 1780, 1872, 1964, 2055, 2155, 2261, 2368, 2475, 2582,
	    2697, 2819, 2942, 3065, 3188, 3320, 3459, 3599, 3739, 3879,
	    4028, 4185, 4343, 4500, 4658, 4825, 5000, 5176, 5352, 5528,
	    5714, 5909, 6104, 6299, 6494, 6699, 6913, 7128, 7342, 7557,
	    7782, 8017, 8251, 8486, 8721, 8966, 9221, 9477, 9732, 9988,
	    10254, 10530, 10807, 11083, 11360, 11647, 11945, 12243, 12540, 12838,
	    13147, 13467, 13786, 14106, 14425, 14756, 15097, 15439, 15780, 16122,
	    16475, 16838, 17202, 17565, 17929, 18304, 18689, 19075, 19461, 19847,
	    20244, 20652, 21060, 21468, 21876, 22296, 22726, 23156, 23587, 24017,
	    24459, 24911, 25364, 25816, 26269, 26732, 27207, 27682, 28156, 28631,
	    29117, 29613, 30110, 30606, 31103, 31611, 32129, 32647, 33165, 33683,
	    34212, 34752, 35292, 35831, 36371, 36921, 37482, 38042, 38603, 39163,
	    39734, 40316, 40897, 41478, 42059, 42650, 43252, 43853, 44454, 45056,
	    45667, 46288, 46909, 47530, 48150, 48781, 49421, 50061, 50701, 51340,
	    51990, 52648, 53306, 53964, 54622, 55290, 55965, 56641, 57317, 57993,
	    58677, 59370, 60063, 60755, 61448, 62149, 62857, 63566, 64275, 64984,
	    65700, 66424, 67148, 67871, 68595, 69326, 70064, 70802, 71540, 72278,
	    73023, 73774, 74525, 75276, 76028, 76785, 77548, 78312, 79075, 79838,
	    80607, 81381, 82155, 82930, 83704, 84483, 85267, 86051, 86835, 87619,
	    88408, 89200, 89993, 90785, 91578, 92374, 93174, 93974, 94774, 95574,
	    96376, 97182, 97988, 98794, 99599
	}
};

/* 8 bit level of an rgbi intensity (0.0 to 1.0) of channel ch. */
static u_int8_t RgbiLevel(int ch, double v)
{
	int i = (int)floor((v * 100000.0) + 0.5), lo = 0, hi = 255;

	/* Number of thresholds <= i. */
	while(lo < hi)
	{
	    int mid = (lo + hi) / 2;

	    if(rgbi_threshold[ch][mid] <= i)
		lo = mid + 1;
	    else
		hi = mid;
	}
	return((u_int8_t)lo);
}

/* Parses an X color spec (rgbi:, #hex, black, white). */
static bool ParseColor(const char *clsp, u_int8_t *r, u_int8_t *g, u_int8_t *b)
{
	double fr, fg, fb;
	unsigned int ir, ig, ib;
	size_t len, n;

	if(sscanf(clsp, "rgbi:%lf/%lf/%lf", &fr, &fg, &fb) == 3)
	{
	    if((fr < 0.0) || (fr > 1.0) || (fg < 0.0) || (fg > 1.0) ||
	       (fb < 0.0) || (fb > 1.0)
	    )
		return(false);
	    *r = RgbiLevel(0, fr);
	    *g = RgbiLevel(1, fg);
	    *b = RgbiLevel(2, fb);
	    return(true);
	}
	if(clsp[0] == '#')
	{
	    len = strlen(clsp + 1);
	    if((len % 3) || (len == 0) || (len > 12))
		return(false);
	    n = len / 3;
	    char fmt[32];
	    sprintf(fmt, "%%%zux%%%zux%%%zux", n, n, n);
	    if(sscanf(clsp + 1, fmt, &ir, &ig, &ib) != 3)
		return(false);
	    /* Scale to 16 bits as X does, then take the high byte. */
	    *r = (u_int8_t)((ir << (16 - (4 * n))) >> 8);
	    *g = (u_int8_t)((ig << (16 - (4 * n))) >> 8);
	    *b = (u_int8_t)((ib << (16 - (4 * n))) >> 8);
	    if(n == 1)
	    {
		*r |= *r >> 4;
		*g |= *g >> 4;
		*b |= *b >> 4;
	    }
	    return(true);
	}
	if(!strcasecmp(clsp, "black"))
	{
	    *r = *g = *b = 0x00;
	    return(true);
	}
	if(!strcasecmp(clsp, "white"))
	{
	    *r = *g = *b = 0xff;
	    return(true);
	}
	return(false);
}

int OSWLoadPixelCLSP(pixel_t *pix_rtn, const char *clsp)
{
	u_int8_t r, g, b;

	if(pix_rtn == NULL)
	    return(-1);
	if(!IDC() || (clsp == NULL))
	{
	    *pix_rtn = 0;
	    return(-1);
	}
	if(!ParseColor(clsp, &r, &g, &b))
	{
	    fprintf(stderr, "OSWLoadPixelCLSP(): %s: BadColor\n", clsp);
	    *pix_rtn = 0;
	    return(-1);
	}
	return(OSWLoadPixelRGB(pix_rtn, r, g, b));
}

void OSWDestroyPixel(pixel_t *pix_ptr)
{
	if(!IDC() || (pix_ptr == NULL))
	    return;
	if((*pix_ptr == osw_gui[0].black_pix) ||
	   (*pix_ptr == osw_gui[0].white_pix)
	)
	    return;
	if(osw_gui[0].current_fg_pix == *pix_ptr)
	    osw_gui[0].current_fg_pix = 0;
	*pix_ptr = 0;
}

void OSWSetFgPix(pixel_t pix)
{
	if(!IDC())
	    return;
	osw_gui[0].current_fg_pix = pix;
}


/* ******************************************************************
 *
 *	XPM (only what the cursors and icons of the program use: one
 *	or two characters per pixel, colors by "c" as #hex, None,
 *	black or white).
 */
struct xpm {
	int width, height;
	std::vector<u_int32_t> pixel;	/* 0x00RRGGBB. */
	std::vector<bool> opaque;
};

static bool ParseXpm(const char **data, int nlines, xpm *x)
{
	int ncolors, cpp, i, j, k;
	std::vector<std::string> keys;
	std::vector<u_int32_t> colors;
	std::vector<bool> opaque;

	if((nlines < 1) || (data[0] == NULL) ||
	   (sscanf(data[0], "%d %d %d %d", &x->width, &x->height, &ncolors, &cpp) != 4) ||
	   (x->width <= 0) || (x->height <= 0) || (ncolors <= 0) ||
	   (cpp <= 0) || (nlines < 1 + ncolors + x->height)
	)
	    return(false);

	for(i = 0; i < ncolors; i++)
	{
	    const char *line = data[1 + i], *s;
	    char spec[64];
	    u_int8_t r = 0, g = 0, b = 0;
	    bool o = true;

	    if((line == NULL) || ((int)strlen(line) < cpp))
		return(false);
	    keys.push_back(std::string(line, cpp));

	    /* Find the "c" (color visual) key. */
	    spec[0] = '\0';
	    for(s = line + cpp; *s != '\0'; s++)
	    {
		if(((*s == ' ') || (*s == '\t')) && (s[1] == 'c') &&
		   ((s[2] == ' ') || (s[2] == '\t'))
		)
		{
		    sscanf(s + 3, "%63s", spec);
		    break;
		}
	    }
	    if(!strcasecmp(spec, "None"))
		o = false;
	    else if(!ParseColor(spec, &r, &g, &b))
		r = g = b = 0;
	    colors.push_back(((u_int32_t)r << 16) | ((u_int32_t)g << 8) | b);
	    opaque.push_back(o);
	}

	x->pixel.assign(x->width * x->height, 0);
	x->opaque.assign(x->width * x->height, false);
	for(j = 0; j < x->height; j++)
	{
	    const char *line = data[1 + ncolors + j];

	    if((line == NULL) || ((int)strlen(line) < x->width * cpp))
		return(false);
	    for(i = 0; i < x->width; i++)
	    {
		for(k = 0; k < ncolors; k++)
		{
		    if(!strncmp(line + (i * cpp), keys[k].c_str(), cpp))
		    {
			x->pixel[(j * x->width) + i] = colors[k];
			x->opaque[(j * x->width) + i] = opaque[k];
			break;
		    }
		}
	    }
	}
	return(true);
}

/* Number of lines of XPM data (from its header). */
static int XpmDataLines(const char **data)
{
	int w, h, ncolors, cpp;

	if((data == NULL) || (data[0] == NULL) ||
	   (sscanf(data[0], "%d %d %d %d", &w, &h, &ncolors, &cpp) != 4)
	)
	    return(0);
	return(1 + ncolors + h);
}

/* Reads the strings of an XPM file into lines. */
static bool ReadXpmFile(const char *path, std::vector<std::string> *lines)
{
	FILE *fp = fopen(path, "rb");
	std::string s;
	int c;
	bool in = false;

	if(fp == NULL)
	    return(false);
	while((c = fgetc(fp)) != EOF)
	{
	    if(c == '"')
	    {
		if(in)
		    lines->push_back(s);
		s.clear();
		in = !in;
	    }
	    else if(in)
		s += (char)c;
	}
	fclose(fp);
	return(!lines->empty());
}

static bool LoadXpmFile(const char *path, xpm *x)
{
	std::vector<std::string> lines;
	std::vector<const char *> p;
	size_t i;

	if(!ReadXpmFile(path, &lines))
	    return(false);
	for(i = 0; i < lines.size(); i++)
	    p.push_back(lines[i].c_str());
	return(ParseXpm(&p[0], (int)p.size(), x));
}


/* ******************************************************************
 *
 *	Pointer cursors.
 */
namespace static_osw_sdl {

static cursor_t AddCursor(SDL_Cursor *c)
{
	if(c == NULL)
	    return(0);
	cursors.push_back(c);
	return(cursors.size());
}

static SDL_Cursor *GetCursor(cursor_t c)
{
	if((c == 0) || (c > cursors.size()))
	    return(NULL);
	return(cursors[c - 1]);
}

/*   Sets the cursor of the grab, else of the window under the
 *   pointer (or its nearest ancestor that has one).
 */
static void UpdateCursor(void)
{
	SDL_Cursor *c = NULL;
	win_t w;
	obj *o;

	if(W(grab_win) != NULL)
	    c = GetCursor(grab_cursor);
	for(w = pointer_win; (c == NULL) && ((o = W(w)) != NULL) && (w != root_win); w = o->parent)
	    c = GetCursor(o->cursor);
	if(c == NULL)
	    c = GetCursor(osw_gui[0].std_cursor);
	if(c == NULL)
	    c = SDL_GetDefaultCursor();
	if(c != current_cursor)
	{
	    SDL_SetCursor(c);
	    current_cursor = c;
	}
}

/*   Creates a cursor from 1 bit source and mask bitmaps (as X's
 *   XCreatePixmapCursor()): source 1 is the foreground color, 0 the
 *   background color, mask 0 is transparent.
 */
static cursor_t CreateBitmapCursor(
	const std::vector<bool> &source, const std::vector<bool> &mask,
	int width, int height, int hot_x, int hot_y,
	u_int32_t fg, u_int32_t bg
)
{
	SDL_Surface *s;
	SDL_Cursor *c;
	int x, y;

	s = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
	if(s == NULL)
	    return(0);
	for(y = 0; y < height; y++)
	{
	    Uint32 *row = (Uint32 *)((Uint8 *)s->pixels + (y * s->pitch));

	    for(x = 0; x < width; x++)
	    {
		int i = (y * width) + x;

		row[x] = mask[i] ?
		    (0xff000000 | (source[i] ? fg : bg)) : 0x00000000;
	    }
	}
	c = SDL_CreateColorCursor(s, hot_x, hot_y);
	SDL_FreeSurface(s);
	return(AddCursor(c));
}

/*   Cursor from XPM, as osw-x.cpp makes it: the XPM is read into a
 *   1 bit pixmap, so light colors are the foreground and dark ones
 *   the (black) background.
 */
static cursor_t CursorFromXpm(
	const xpm *x, int *hot_x, int *hot_y,
	u_int8_t r, u_int8_t g, u_int8_t b,
	unsigned int *width, unsigned int *height
)
{
	std::vector<bool> source(x->width * x->height), mask(x->width * x->height);
	size_t i;

	for(i = 0; i < source.size(); i++)
	{
	    u_int32_t p = x->pixel[i];
	    int lum = ((p >> 16) & 0xff) + ((p >> 8) & 0xff) + (p & 0xff);

	    source[i] = (lum >= (0xff * 3) / 2);
	    mask[i] = x->opaque[i];
	}

	/* Sanitize hot point. */
	if(*hot_x >= x->width) *hot_x = x->width - 1;
	if(*hot_x < 0) *hot_x = 0;
	if(*hot_y >= x->height) *hot_y = x->height - 1;
	if(*hot_y < 0) *hot_y = 0;

	*width = x->width;
	*height = x->height;
	return(CreateBitmapCursor(
	    source, mask, x->width, x->height, *hot_x, *hot_y,
	    ((u_int32_t)r << 16) | ((u_int32_t)g << 8) | b, 0x000000
	));
}

}	/* namespace static_osw_sdl */

cursor_t OSWLoadBasicCursor(cur_code_t code)
{
	SDL_SystemCursor id;

	if(!IDC())
	    return(0);
	switch(code)
	{
	  case XC_xterm: id = SDL_SYSTEM_CURSOR_IBEAM; break;
	  case XC_watch: id = SDL_SYSTEM_CURSOR_WAIT; break;
	  case XC_crosshair: id = SDL_SYSTEM_CURSOR_CROSSHAIR; break;
	  case XC_fleur: id = SDL_SYSTEM_CURSOR_SIZEALL; break;
	  case XC_sb_h_double_arrow: id = SDL_SYSTEM_CURSOR_SIZEWE; break;
	  case XC_sb_v_double_arrow: id = SDL_SYSTEM_CURSOR_SIZENS; break;
	  case XC_hand2: id = SDL_SYSTEM_CURSOR_HAND; break;
	  case XC_X_cursor: id = SDL_SYSTEM_CURSOR_NO; break;
	  default: id = SDL_SYSTEM_CURSOR_ARROW; break;
	}
	return(AddCursor(SDL_CreateSystemCursor(id)));
}

void OSWSetWindowCursor(win_t w, cursor_t cursor)
{
	obj *o = W(w);

	if(!IDC() || (o == NULL) || (cursor == 0))
	    return;
	o->cursor = cursor;
	UpdateCursor();
}

void OSWUnsetWindowCursor(win_t w)
{
	obj *o = W(w);

	if(!IDC() || (o == NULL))
	    return;
	o->cursor = 0;
	UpdateCursor();
}

void OSWDestroyCursor(cursor_t *cursor)
{
	SDL_Cursor *c;
	size_t i;

	if(!IDC() || (cursor == NULL) || (*cursor == 0))
	    return;
	c = GetCursor(*cursor);
	if(c != NULL)
	{
	    for(i = 1; i < objs.size(); i++)
	    {
		if((objs[i] != NULL) && (objs[i]->cursor == *cursor))
		    objs[i]->cursor = 0;
	    }
	    if(grab_cursor == *cursor)
		grab_cursor = 0;
	    if(current_cursor == c)
	    {
		SDL_SetCursor(SDL_GetDefaultCursor());
		current_cursor = NULL;
	    }
	    SDL_FreeCursor(c);
	    cursors[*cursor - 1] = NULL;
	}
	*cursor = 0;
	UpdateCursor();
}

cursor_t OSWCreateCursorFromXpmFile(
	char *xpmfile,
	int *hot_x, int *hot_y,
	u_int8_t r, u_int8_t g, u_int8_t b,
	unsigned int *width, unsigned int *height
)
{
	xpm x;
	struct stat stat_buf;

	if(!IDC() || (xpmfile == NULL))
	    return(0);
	if(stat(xpmfile, &stat_buf))
	{
	    fprintf(stderr, "%s: No such file.\n", xpmfile);
	    return(0);
	}
	if(!LoadXpmFile(xpmfile, &x))
	{
	    fprintf(stderr, "%s: Unable to load Pixmap.\n", xpmfile);
	    return(0);
	}
	return(CursorFromXpm(&x, hot_x, hot_y, r, g, b, width, height));
}

cursor_t OSWCreateCursorFromXpmData(
	const char **xpmdata,
	int *hot_x, int *hot_y,
	u_int8_t r, u_int8_t g, u_int8_t b,
	unsigned int *width, unsigned int *height
)
{
	xpm x;

	if(!IDC() || (xpmdata == NULL))
	    return(0);
	if(!ParseXpm(xpmdata, XpmDataLines(xpmdata), &x))
	{
	    fprintf(stderr, "%p: Unable to load embedded Pixmap.\n",
		(const void *)xpmdata
	    );
	    return(0);
	}
	return(CursorFromXpm(&x, hot_x, hot_y, r, g, b, width, height));
}

cursor_t OSWCreateCursorFromImage(
	image_t *image,
	unsigned int *width_rtn, unsigned int *height_rtn
)
{
	int x, y, w, h;
	std::vector<bool> source, mask;

	if(!IDC() || (image == NULL) || (image->data == NULL) ||
	   (image->width <= 0) || (image->height <= 0)
	)
	    return(0);

	/*   As osw-x.cpp: bright pixels are white, others black, and
	 *   pixels that are not 0 are shown. The hot point is the center.
	 */
	w = image->width;
	h = image->height;
	source.resize(w * h);
	mask.resize(w * h);
	for(y = 0; y < h; y++)
	{
	    const u_int32_t *row = (const u_int32_t *)(image->data + (y * image->bytes_per_line));

	    for(x = 0; x < w; x++)
	    {
		u_int32_t p = row[x];
		int i = ((p >> 16) & 0xff) + ((p >> 8) & 0xff) + (p & 0xff);

		source[(y * w) + x] = (i >= ((0xFF * 3) / 2));
		mask[(y * w) + x] = (p != 0);
	    }
	}
	*width_rtn = w;
	*height_rtn = h;
	return(CreateBitmapCursor(source, mask, w, h, w / 2, h / 2, 0xffffff, 0x000000));
}


/* ******************************************************************
 *
 *	Events.
 */
int OSWEventsPending(void)
{
	if(!IDC())
	    return(0);
	Pump();
	PresentAll();
	return((int)queue.size());
}

void OSWWaitNextEvent(event_t *event)
{
	if(event == NULL)
	    return;
	memset(event, 0x00, sizeof(event_t));
	if(!IDC())
	    return;
	WaitForEvent();
	if(queue.empty())
	    return;
	*event = queue.front();
	queue.pop_front();
	ManageEvent(event);
}

void OSWWaitPeakEvent(event_t *event)
{
	if(event == NULL)
	    return;
	memset(event, 0x00, sizeof(event_t));
	if(!IDC())
	    return;
	WaitForEvent();
	if(queue.empty())
	    return;
	*event = queue.front();
	ManageEvent(event);
}

/* Removes the first queued event matching w (0 any) and mask. */
static bool TakeEvent(win_t w, eventmask_t mask, event_t *event)
{
	std::deque<event_t>::iterator i;

	for(i = queue.begin(); i != queue.end(); ++i)
	{
	    if(((w == 0) || (i->xany.window == w)) && Selected(mask, &(*i)))
	    {
		*event = *i;
		queue.erase(i);
		ManageEvent(event);
		return(true);
	    }
	}
	return(false);
}

bool_t OSWCheckMaskEvent(eventmask_t eventmask, event_t *event)
{
	if(!IDC() || (event == NULL))
	    return(False);
	Pump();
	return(TakeEvent(0, eventmask, event) ? True : False);
}

void OSWWaitWindowEvent(win_t w, eventmask_t event_mask, event_t *event)
{
	SDL_Event se;

	if(event == NULL)
	    return;
	memset(event, 0x00, sizeof(event_t));
	if(!IDC() || (w == 0))
	    return;
	Pump();
	while(!TakeEvent(w, event_mask, event) && IDC())
	{
	    PresentAll();
	    if(SDL_WaitEventTimeout(&se, 100))
		Translate(&se);
	    Pump();
	}
}

bool_t OSWCheckWindowEvent(win_t w, eventmask_t event_mask, event_t *event)
{
	if(event == NULL)
	    return(False);
	memset(event, 0x00, sizeof(event_t));
	if(!IDC() || (w == 0))
	    return(False);
	Pump();
	return(TakeEvent(w, event_mask, event) ? True : False);
}

void OSWPutBackEvent(event_t *event)
{
	if(!IDC() || (event == NULL))
	    return;
	queue.push_front(*event);
}

int OSWSendEvent(eventmask_t mask, event_t *event, bool_t propagate)
{
	event_t ev;
	win_t w;
	obj *o;

	if(!IDC() || (event == NULL) || (event->xany.window == 0))
	    return(-1);

	ev = *event;
	ev.xany.send_event = True;
	ev.xany.display = osw_gui[0].display;

	/*   As XSendEvent(): with no mask it goes to the window's
	 *   client (always us), else to the window if it selected any
	 *   of mask, else (if propagate) to the nearest ancestor that did.
	 */
	if(mask == 0)
	{
	    Queue(&ev);
	    return(0);
	}
	for(w = event->xany.window; ((o = W(w)) != NULL) && (w != root_win); w = o->parent)
	{
	    if(o->event_mask & mask)
	    {
		ev.xany.window = w;
		Queue(&ev);
		break;
	    }
	    if(!propagate)
		break;
	}
	return(0);
}

int OSWPurgeAllEvents(void)
{
	int i;

	if(!IDC())
	    return(0);
	Pump();
	i = (int)queue.size();
	queue.clear();
	return(i);
}

int OSWPurgeOldMotionEvents(void)
{
	std::deque<event_t>::iterator i;
	event_t last;
	int n = 0;

	if(!IDC())
	    return(0);
	Pump();
	for(i = queue.begin(); i != queue.end(); )
	{
	    if(i->type == MotionNotify)
	    {
		last = *i;
		i = queue.erase(i);
		n++;
	    }
	    else
		++i;
	}
	/* Put the last one back (at the front, as XPutBackEvent()). */
	if(n > 0)
	    queue.push_front(last);
	return(n);
}

int OSWPurgeTypedEvent(eventtype_t event_type)
{
	std::deque<event_t>::iterator i;
	int n = 0;

	if(!IDC())
	    return(0);
	Pump();
	for(i = queue.begin(); i != queue.end(); )
	{
	    if(i->type == event_type)
	    {
		i = queue.erase(i);
		n++;
	    }
	    else
		++i;
	}
	return(n);
}

int OSWPurgeWindowTypedEvent(win_t w, eventtype_t event_type)
{
	std::deque<event_t>::iterator i;
	int n = 0;

	if(!IDC() || (w == 0))
	    return(0);
	Pump();
	for(i = queue.begin(); i != queue.end(); )
	{
	    if((i->type == event_type) && (i->xany.window == w))
	    {
		i = queue.erase(i);
		n++;
	    }
	    else
		++i;
	}
	return(n);
}

int OSWIsEventDestroyWindow(win_t w, event_t *event)
{
	if(!IDC() || (w == 0) || (event == NULL))
	    return(0);
	if((event->type == ClientMessage) &&
	   (event->xclient.format == 32) &&
	   ((atom_t)event->xclient.data.l[0] == osw_atom.wm_delete_window) &&
	   (event->xany.window == w)
	)
	    return(1);
	return(0);
}


/* ******************************************************************
 *
 *	Windows.
 */
static int CreateWindowCommon(
	win_t *w, win_t parent, int x, int y,
	unsigned int width, unsigned int height, bool input_only
)
{
	obj *p = W(parent), *o;

	if(!IDC() || (w == NULL) || (p == NULL) || (width == 0) || (height == 0))
	    return(-1);

	*w = NewObj(OBJ_WINDOW);
	o = W(*w);
	o->parent = parent;
	o->x = x;
	o->y = y;
	o->width = width;
	o->height = height;
	o->input_only = input_only;
	o->bkg_pix = osw_gui[0].black_pix;
	o->frame_style = WindowFrameStyleStandard;
	if(!input_only)
	{
	    o->data = (u_int32_t *)calloc(width * height, 4);
	    if(o->data == NULL)
	    {
		o->type = OBJ_FREE;
		*w = 0;
		return(-1);
	    }
	}
	p->children.push_back(*w);
	return(0);
}

int OSWCreateWindow(
	win_t *w, win_t parent, int x, int y,
	unsigned int width, unsigned int height
)
{
	return(CreateWindowCommon(w, parent, x, y, width, height, false));
}

int OSWCreateInputWindow(
	win_t *w, win_t parent, int x, int y,
	unsigned int width, unsigned int height
)
{
	return(CreateWindowCommon(w, parent, x, y, width, height, true));
}

static void RemoveChild(win_t parent, win_t w)
{
	obj *p = W(parent);
	size_t i;

	if(p == NULL)
	    return;
	for(i = 0; i < p->children.size(); i++)
	{
	    if(p->children[i] == w)
	    {
		p->children.erase(p->children.begin() + i);
		return;
	    }
	}
}

static void DestroySDLWindow(obj *o)
{
	if(o->texture != NULL)
	    SDL_DestroyTexture(o->texture);
	if(o->renderer != NULL)
	    SDL_DestroyRenderer(o->renderer);
	if(o->sdl_win != NULL)
	    SDL_DestroyWindow(o->sdl_win);
	o->texture = NULL;
	o->renderer = NULL;
	o->sdl_win = NULL;
	free(o->comp);
	o->comp = NULL;
}

static void DestroyTree(win_t w)
{
	obj *o = W(w);

	if(o == NULL)
	    return;
	while(!o->children.empty())
	{
	    win_t c = o->children.back();

	    o->children.pop_back();
	    DestroyTree(c);
	}
	DestroySDLWindow(o);
	free(o->data);
	o->data = NULL;
	o->type = OBJ_FREE;
	if(focus_win == w) focus_win = 0;
	if(pointer_win == w) pointer_win = 0;
	if(grab_win == w) ReleaseGrab();
	if(main_win == w) main_win = 0;
}

void OSWDestroyWindow(win_t *w)
{
	obj *o;

	if(!IDC() || (w == NULL))
	    return;
	o = W(*w);
	if((o != NULL) && (*w != root_win))
	{
	    MarkDirty(o->parent);
	    RemoveChild(o->parent, *w);
	    DestroyTree(*w);
	}
	*w = 0;
}

bool_t OSWDrawableIsWindow(drawable_t d)
{
	return((W(d) != NULL) ? True : False);
}

void OSWSetWindowWMProperties(
	win_t w,
	const char *title,
	const char *icon_title,
	pixmap_t icon,
	bool_t wm_sets_coordinates,
	int x, int y,
	unsigned int min_width, unsigned int min_height,
	unsigned int max_width, unsigned int max_height,
	int frame_style,
	char **argv, int argc
)
{
	obj *o = W(w);

	if(!IDC() || (o == NULL))
	    return;

	if(title == NULL)
	    title = "Untitled";

	/* Sanitize sizes as osw-x.cpp. */
	if(max_width > osw_gui[0].display_width)
	    max_width = osw_gui[0].display_width;
	if(max_height > osw_gui[0].display_height)
	    max_height = osw_gui[0].display_height;
	if(max_width < 1)
	    max_width = 1;
	if(max_height < 1)
	    max_height = 1;
	if(min_width > max_width)
	    min_width = max_width;
	if(min_height > max_height)
	    min_height = max_height;

	o->title = title;
	o->icon = icon;
	o->frame_style = frame_style;
	o->min_width = min_width;
	o->min_height = min_height;
	o->max_width = max_width;
	o->max_height = max_height;

	/* Applied when the SDL window is created, or now if it is. */
	if(o->sdl_win != NULL)
	{
	    SDL_SetWindowTitle(o->sdl_win, title);
	    SDL_SetWindowMinimumSize(o->sdl_win, MAX(min_width, 1), MAX(min_height, 1));
	    SDL_SetWindowMaximumSize(o->sdl_win, MAX(max_width, 1), MAX(max_height, 1));
	}
}

void OSWSetWindowInput(win_t w, eventmask_t eventmask)
{
	obj *o = W(w);

	if(!IDC() || (o == NULL))
	    return;
	o->event_mask = eventmask;
	if(grab_win == w && !grab_explicit)
	    grab_mask = eventmask;
}

void OSWSetTransientFor(win_t wbum, win_t wshelter)
{
	/* No equivalent in SDL2 that is not modal. */
}

void OSWSetWindowTitle(win_t w, const char *title)
{
	obj *o = W(w);

	if(!IDC() || (o == NULL) || (title == NULL))
	    return;
	o->title = title;
	if(o->sdl_win != NULL)
	    SDL_SetWindowTitle(o->sdl_win, title);
}

void OSWClearWindow(win_t w)
{
	if(!IDC())
	    return;
	ClearToBkg(w);
}

void OSWSetWindowBkg(win_t w, pixel_t pix, pixmap_t pixmap)
{
	obj *o = W(w);

	if(!IDC() || (o == NULL))
	    return;
	if(pixmap == 0)
	{
	    o->bkg_pix = pix;
	    o->bkg_pixmap = 0;
	}
	else
	    o->bkg_pixmap = pixmap;
}

bool_t OSWGetWindowAttributes(win_t w, win_attr_t *wattr)
{
	obj *o = W(w);

	if(!IDC() || (wattr == NULL))
	    return(False);
	memset(wattr, 0x00, sizeof(win_attr_t));
	if(o == NULL)
	    return(False);
	wattr->x = o->x;
	wattr->y = o->y;
	wattr->width = o->width;
	wattr->height = o->height;
	wattr->depth = o->input_only ? 0 : osw_gui[0].depth;
	wattr->root = root_win;
	if(!o->mapped)
	    wattr->map_state = IsUnmapped;
	else
	    wattr->map_state = Viewable(w) ? IsViewable : IsUnviewable;
	wattr->your_event_mask = o->event_mask;
	return(True);
}

int OSWGetWindowRootPos(win_t w, int *x, int *y)
{
	int rx, ry;

	if(!IDC() || (W(w) == NULL))
	    return(-1);
	RootPos(w, &rx, &ry);
	if(x != NULL) *x = rx;
	if(y != NULL) *y = ry;
	return(0);
}

win_t OSWGetWindowParent(win_t w)
{
	obj *o = W(w);

	if(!IDC() || (o == NULL))
	    return(0);
	if(w == root_win)
	    return(root_win);
	return(o->parent);
}

bool_t OSWCheckWindowAncestory(win_t grand_parent, win_t grand_child)
{
	if(!IDC())
	    return(False);
	if(grand_parent == grand_child)
	    return(True);
	if((grand_child == root_win) || (grand_child == 0) ||
	   (grand_parent == root_win) || (grand_parent == 0)
	)
	    return(False);
	return(IsDescendant(grand_parent, grand_child) ? True : False);
}

void OSWSetMainWindow(win_t w)
{
	main_win = w;
}

win_t OSWGetMainWindow(void)
{
	return(main_win);
}

/* Creates the SDL window of toplevel w, hidden. */
static bool CreateSDLWindow(win_t w)
{
	obj *o = W(w), *icon;
	Uint32 flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI;

	if(o->sdl_win != NULL)
	    return(true);

	if(o->frame_style == WindowFrameStyleNaked)
	    flags |= SDL_WINDOW_BORDERLESS | SDL_WINDOW_SKIP_TASKBAR |
		SDL_WINDOW_POPUP_MENU;
	else if((o->max_width > o->min_width) || (o->max_height > o->min_height))
	{
	    if((o->frame_style == WindowFrameStyleStandard) ||
	       (o->frame_style == WindowFrameStyleNoClose)
	    )
		flags |= SDL_WINDOW_RESIZABLE;
	}

	o->sdl_win = SDL_CreateWindow(
	    o->title.c_str(), o->x, o->y, o->width, o->height, flags
	);
	if(o->sdl_win == NULL)
	{
	    fprintf(stderr, "Cannot create window: %s\n", SDL_GetError());
	    return(false);
	}
	o->renderer = SDL_CreateRenderer(o->sdl_win, -1, 0);
	if(o->renderer == NULL)
	    o->renderer = SDL_CreateRenderer(o->sdl_win, -1, SDL_RENDERER_SOFTWARE);
	if(o->renderer == NULL)
	{
	    fprintf(stderr, "Cannot create renderer: %s\n", SDL_GetError());
	    DestroySDLWindow(o);
	    return(false);
	}
	SDL_RenderSetIntegerScale(o->renderer, SDL_TRUE);
	if(o->max_width > 0)
	{
	    SDL_SetWindowMinimumSize(o->sdl_win, MAX(o->min_width, 1), MAX(o->min_height, 1));
	    SDL_SetWindowMaximumSize(o->sdl_win, MAX(o->max_width, 1), MAX(o->max_height, 1));
	}

	icon = O(o->icon);
	if((icon != NULL) && (icon->data != NULL))
	{
	    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(
		icon->data, icon->width, icon->height, 32, icon->width * 4,
		SDL_PIXELFORMAT_RGB888
	    );
	    if(s != NULL)
	    {
		SDL_SetWindowIcon(o->sdl_win, s);
		SDL_FreeSurface(s);
	    }
	}
	return(true);
}

static void MapCommon(win_t w, bool raise)
{
	obj *o = W(w), *p;

	if(!IDC() || (o == NULL) || (w == root_win))
	    return;

	p = W(o->parent);
	if(raise && (p != NULL))
	{
	    RemoveChild(o->parent, w);
	    p->children.push_back(w);
	    MarkDirty(w);
	}
	if(o->mapped)
	{
	    if(raise && (o->parent == root_win) && (o->sdl_win != NULL))
		SDL_RaiseWindow(o->sdl_win);
	    return;
	}

	o->mapped = true;
	if(o->parent == root_win)
	{
	    if(!CreateSDLWindow(w))
	    {
		o->mapped = false;
		return;
	    }
	    o->iconified = false;
	    SDL_ShowWindow(o->sdl_win);
	    if(raise)
		SDL_RaiseWindow(o->sdl_win);
	    o->dirty = true;
	}
	SendStructure(w, MapNotify);
	ExposeTree(w);
	MarkDirty(w);
	SetPointer(Toplevel(pointer_win), pointer_x, pointer_y);
}

void OSWMapWindow(win_t w)
{
	MapCommon(w, false);
}

void OSWMapRaised(win_t w)
{
	MapCommon(w, true);
}

void OSWMapSubwindows(win_t w)
{
	obj *o = W(w);
	int i;

	if(!IDC() || (o == NULL))
	    return;
	/* X maps them from the top of the stack down. */
	for(i = (int)o->children.size() - 1; i >= 0; i--)
	    MapCommon(o->children[i], false);
}

void OSWUnmapWindow(win_t w)
{
	obj *o = W(w);

	if(!IDC() || (o == NULL) || !o->mapped || (w == root_win))
	    return;
	o->mapped = false;
	if((o->parent == root_win) && (o->sdl_win != NULL))
	    SDL_HideWindow(o->sdl_win);
	MarkDirty(o->parent);
	SendStructure(w, UnmapNotify);
	if((grab_win != 0) && !Viewable(grab_win))
	    ReleaseGrab();
	if((pointer_win != 0) && !Viewable(pointer_win))
	    SetPointer(Toplevel(o->parent == root_win ? 0 : w), pointer_x, pointer_y);
}

void OSWRestackWindows(win_t *w, int num_w)
{
	int i;

	if(!IDC() || (w == NULL) || (num_w <= 0))
	    return;

	/*   Each window goes just below the one before it (siblings
	 *   only; toplevels are stacked by the window manager).
	 */
	for(i = 1; i < num_w; i++)
	{
	    obj *a = W(w[i - 1]), *b = W(w[i]), *p;
	    size_t k;

	    if((a == NULL) || (b == NULL) || (a->parent != b->parent) ||
	       (a->parent == root_win)
	    )
		continue;
	    p = W(a->parent);
	    RemoveChild(a->parent, w[i]);
	    for(k = 0; k < p->children.size(); k++)
	    {
		if(p->children[k] == w[i - 1])
		{
		    p->children.insert(p->children.begin() + k, w[i]);
		    break;
		}
	    }
	    MarkDirty(w[i]);
	}
}

int OSWIconifyWindow(win_t w)
{
	obj *o = W(w);

	if(!IDC() || (o == NULL))
	    return(-1);
	if(o->sdl_win != NULL)
	    SDL_MinimizeWindow(o->sdl_win);
	return(0);
}

void OSWReparentWindow(win_t w, win_t parent)
{
	obj *o = W(w), *p = W(parent);

	if(!IDC() || (o == NULL) || (p == NULL) || (w == root_win) ||
	   IsDescendant(w, parent)
	)
	    return;

	MarkDirty(w);
	RemoveChild(o->parent, w);
	if(o->parent == root_win)
	    DestroySDLWindow(o);
	o->parent = parent;
	o->x = 0;
	o->y = 0;
	p->children.push_back(w);
	if(o->mapped)
	{
	    if(parent == root_win)
	    {
		o->mapped = false;
		MapCommon(w, false);
	    }
	    else
		ExposeTree(w);
	}
	MarkDirty(w);
}

static void ConfigureCommon(win_t w, int x, int y, int width, int height)
{
	obj *o = W(w);
	bool moved, resized;

	if(!IDC() || (o == NULL) || (w == root_win) || (width <= 0) || (height <= 0))
	    return;
	moved = (x != o->x) || (y != o->y);
	resized = (width != o->width) || (height != o->height);
	if(!moved && !resized)
	    return;

	MarkDirty(w);
	o->x = x;
	o->y = y;
	if(resized)
	    ResizeBuffer(w, width, height);
	if(o->sdl_win != NULL)
	{
	    if(moved)
		SDL_SetWindowPosition(o->sdl_win, x, y);
	    if(resized)
		SDL_SetWindowSize(o->sdl_win, width, height);
	}
	MarkDirty(w);
	SendConfigure(w);
	if(resized)
	    ExposeTree(w);
}

void OSWMoveWindow(win_t w, int x, int y)
{
	obj *o = W(w);

	if(o != NULL)
	    ConfigureCommon(w, x, y, o->width, o->height);
}

void OSWResizeWindow(win_t w, unsigned int width, unsigned int height)
{
	obj *o = W(w);

	if(o != NULL)
	    ConfigureCommon(w, o->x, o->y, (int)width, (int)height);
}

void OSWMoveResizeWindow(
	win_t w, int x, int y,
	unsigned int width, unsigned int height
)
{
	ConfigureCommon(w, x, y, (int)width, (int)height);
}


/* ******************************************************************
 *
 *	Pixmaps.
 */
int OSWCreatePixmap(pixmap_t *pixmap, unsigned int width, unsigned int height)
{
	obj *o;

	if(!IDC() || (pixmap == NULL) || (width == 0) || (height == 0))
	    return(-1);
	*pixmap = NewObj(OBJ_PIXMAP);
	o = O(*pixmap);
	o->width = width;
	o->height = height;
	o->data = (u_int32_t *)calloc(width * height, 4);
	if(o->data == NULL)
	{
	    o->type = OBJ_FREE;
	    *pixmap = 0;
	    return(-1);
	}
	return(0);
}

void OSWDestroyPixmap(pixmap_t *pixmap)
{
	obj *o;
	size_t i;

	if(!IDC() || (pixmap == NULL))
	    return;
	o = O(*pixmap);
	if((o != NULL) && (o->type == OBJ_PIXMAP))
	{
	    free(o->data);
	    o->data = NULL;
	    o->type = OBJ_FREE;
	    for(i = 1; i < objs.size(); i++)
	    {
		if((objs[i] != NULL) && (objs[i]->bkg_pixmap == *pixmap))
		    objs[i]->bkg_pixmap = 0;
	    }
	}
	*pixmap = 0;
}

bool_t OSWDrawableIsPixmap(drawable_t d)
{
	obj *o = O(d);

	return(((o != NULL) && (o->type == OBJ_PIXMAP)) ? True : False);
}

bool_t OSWGetPixmapAttributes(pixmap_t pixmap, pixmap_attr_t *pattr)
{
	obj *o = O(pixmap);

	if(!IDC() || (pattr == NULL))
	    return(False);
	memset(pattr, 0x00, sizeof(pixmap_attr_t));
	if((o == NULL) || (o->type != OBJ_PIXMAP))
	    return(False);
	pattr->root_win = root_win;
	pattr->width = o->width;
	pattr->height = o->height;
	pattr->border_width = 0;
	pattr->depth = osw_gui[0].depth;
	return(True);
}

void OSWClearPixmap(
	pixmap_t pixmap,
	unsigned int width, unsigned int height,
	pixel_t pix
)
{
	if(!IDC() || (pixmap == 0) || (width == 0) || (height == 0))
	    return;
	OSWSetFgPix(pix);
	OSWDrawSolidRectangle((drawable_t)pixmap, 0, 0, width, height);
}


/* ******************************************************************
 *
 *	Images.
 */
static image_t *NewImage(unsigned int width, unsigned int height)
{
	image_t *image = (image_t *)calloc(1, sizeof(image_t));

	if(image == NULL)
	    return(NULL);
	image->data = (char *)calloc(width * height, 4);
	if(image->data == NULL)
	{
	    free(image);
	    return(NULL);
	}
	image->width = width;
	image->height = height;
	image->xoffset = 0;
	image->format = 2;		/* ZPixmap */
	image->byte_order = 0;		/* LSBFirst */
	image->bitmap_pad = 32;
	image->depth = osw_gui[0].depth;
	image->bytes_per_line = width * 4;
	image->bits_per_pixel = 32;
	image->red_mask = 0xff0000;
	image->green_mask = 0x00ff00;
	image->blue_mask = 0x0000ff;
	return(image);
}

int OSWCreateImage(image_t **image, unsigned int width, unsigned int height)
{
	if(!IDC() || (image == NULL) || (width == 0) || (height == 0))
	    return(-1);

	/* Width and height must be even number (as osw-x.cpp). */
	if(IS_NUM_ODD(width))
	    width += 1;
	if(IS_NUM_ODD(height))
	    height += 1;

	*image = NewImage(width, height);
	return((*image == NULL) ? -1 : 0);
}

void OSWDestroyImage(image_t **image)
{
	if((image == NULL) || (*image == NULL))
	    return;
	free((*image)->data);
	free(*image);
	*image = NULL;
}

int OSWCreateSharedImage(
	shared_image_t **image, unsigned int width, unsigned int height
)
{
	image_t *ximage_ptr;

	if(!IDC() || (image == NULL) || (width == 0) || (height == 0))
	    return(-1);
	*image = (shared_image_t *)calloc(1, sizeof(shared_image_t));
	if(*image == NULL)
	    return(-1);
	if(OSWCreateImage(&ximage_ptr, width, height))
	{
	    free(*image);
	    *image = NULL;
	    return(-1);
	}
	(*image)->in_progress = False;
	(*image)->ximage = ximage_ptr;
	(*image)->byte_order = ximage_ptr->byte_order;
	(*image)->xoffset = ximage_ptr->xoffset;
	(*image)->format = ximage_ptr->format;
	(*image)->data = (u_int8_t *)ximage_ptr->data;
	(*image)->bytes_per_line = ximage_ptr->bytes_per_line;
	(*image)->bits_per_pixel = ximage_ptr->bits_per_pixel;
	(*image)->bitmap_pad = ximage_ptr->bitmap_pad;
	(*image)->width = ximage_ptr->width;
	(*image)->height = ximage_ptr->height;
	(*image)->depth = ximage_ptr->depth;
	(*image)->red_mask = ximage_ptr->red_mask;
	(*image)->green_mask = ximage_ptr->green_mask;
	(*image)->blue_mask = ximage_ptr->blue_mask;
	return(0);
}

void OSWDestroySharedImage(shared_image_t **image)
{
	if((image == NULL) || (*image == NULL))
	    return;
	OSWDestroyImage(&(*image)->ximage);
	free(*image);
	*image = NULL;
}

pixmap_t OSWCreatePixmapFromImage(image_t *image)
{
	pixmap_t pixmap;

	if(!IDC() || (image == NULL))
	    return(0);
	if(OSWCreatePixmap(&pixmap, image->width, image->height))
	    return(0);
	OSWPutImageToDrawable(image, (drawable_t)pixmap);
	return(pixmap);
}

pixmap_t OSWCreatePixmapMaskFromImage(image_t *image)
{
	pixmap_t pixmap;
	obj *o;
	int x, y;

	if(!IDC() || (image == NULL) || (image->data == NULL))
	    return(0);
	if(OSWCreatePixmap(&pixmap, image->width, image->height))
	    return(0);
	o = O(pixmap);
	for(y = 0; y < image->height; y++)
	{
	    const u_int32_t *row = (const u_int32_t *)(image->data + (y * image->bytes_per_line));

	    for(x = 0; x < image->width; x++)
		o->data[(y * o->width) + x] = row[x] ?
		    (u_int32_t)osw_gui[0].white_pix : (u_int32_t)osw_gui[0].black_pix;
	}
	return(pixmap);
}

static image_t *ImageFromXpm(const xpm *x)
{
	image_t *image;
	int i, j;

	image = NewImage(x->width, x->height);
	if(image == NULL)
	    return(NULL);
	for(j = 0; j < x->height; j++)
	{
	    u_int32_t *row = (u_int32_t *)(image->data + (j * image->bytes_per_line));

	    for(i = 0; i < x->width; i++)
		row[i] = x->pixel[(j * x->width) + i];
	}
	return(image);
}

image_t *OSWLoadImageFromXpmFile(char *filename)
{
	xpm x;

	if(!IDC() || (filename == NULL))
	    return(NULL);
	if(!LoadXpmFile(filename, &x))
	{
	    fprintf(stderr, "OSWLoadImageFromXpmFile(): ");
	    fprintf(stderr, "%s: Failed load.\n", filename);
	    return(NULL);
	}
	return(ImageFromXpm(&x));
}

image_t *OSWLoadImageFromXpmData(char **data)
{
	xpm x;

	if(!IDC() || (data == NULL))
	    return(NULL);
	if(!ParseXpm((const char **)data, XpmDataLines((const char **)data), &x))
	{
	    fprintf(stderr, "OSWLoadImageFromXpmData(): ");
	    fprintf(stderr, "%p: Failed load.\n", (void *)data);
	    return(NULL);
	}
	return(ImageFromXpm(&x));
}

pixmap_t OSWLoadPixmapFromXpmFile(char *filename)
{
	image_t *image = OSWLoadImageFromXpmFile(filename);
	pixmap_t pixmap;

	if(image == NULL)
	    return(0);
	pixmap = OSWCreatePixmapFromImage(image);
	OSWDestroyImage(&image);
	return(pixmap);
}

pixmap_t OSWLoadPixmapFromXpmData(char **data)
{
	image_t *image = OSWLoadImageFromXpmData(data);
	pixmap_t pixmap;

	if(image == NULL)
	    return(0);
	pixmap = OSWCreatePixmapFromImage(image);
	OSWDestroyImage(&image);
	return(pixmap);
}

image_t *OSWGetImage(
	drawable_t d, int x, int y,
	unsigned int width, unsigned int height
)
{
	obj *o = O(d);
	image_t *image;
	buffer b;

	if(!IDC() || (o == NULL) || (width == 0) || (height == 0))
	    return(NULL);
	image = NewImage(width, height);
	if(image == NULL)
	    return(NULL);
	b.data = (u_int32_t *)image->data;
	b.width = width;
	b.height = height;

	/* A window's image includes its subwindows, as on the screen. */
	if(o->type == OBJ_WINDOW)
	    Composite(d, &b, -x, -y, 0, 0, width, height);
	else
	    Blit(&b, 0, 0, o->data, o->width, o->height, o->width * 4,
		x, y, width, height);
	return(image);
}

void OSWPutImageToDrawableSect(
	image_t *image, drawable_t d,
	int tar_x, int tar_y,
	int src_x, int src_y,
	unsigned int width, unsigned int height
)
{
	buffer b;

	if(!IDC() || (image == NULL) || (image->data == NULL) || !Target(d, &b))
	    return;
	Blit(&b, tar_x, tar_y, (const u_int32_t *)image->data,
	    image->width, image->height, image->bytes_per_line,
	    src_x, src_y, width, height);
}

void OSWPutImageToDrawable(image_t *image, drawable_t d)
{
	if(image == NULL)
	    return;
	OSWPutImageToDrawableSect(image, d, 0, 0, 0, 0, image->width, image->height);
}

void OSWPutImageToDrawablePos(image_t *image, drawable_t d, int tar_x, int tar_y)
{
	if(image == NULL)
	    return;
	OSWPutImageToDrawableSect(image, d, tar_x, tar_y, 0, 0,
	    image->width, image->height);
}

void OSWPutSharedImageToDrawable(shared_image_t *image, drawable_t d)
{
	if(image == NULL)
	    return;
	OSWPutImageToDrawable(image->ximage, d);
}

void OSWPutSharedImageToDrawablePos(
	shared_image_t *image, drawable_t d, int tar_x, int tar_y
)
{
	if(image == NULL)
	    return;
	OSWPutImageToDrawablePos(image->ximage, d, tar_x, tar_y);
}

void OSWPutSharedImageToDrawableSect(
	shared_image_t *image, drawable_t d,
	int tar_x, int tar_y,
	int src_x, int src_y,
	unsigned int width, unsigned int height
)
{
	if(image == NULL)
	    return;
	OSWPutImageToDrawableSect(image->ximage, d, tar_x, tar_y,
	    src_x, src_y, width, height);
}

void OSWSyncSharedImage(shared_image_t *image, drawable_t d)
{
	/* Images are put at once. */
}

void OSWPutBufferToWindow(win_t w, gbuf_t gbuf)
{
	obj *o = W(w);

	if(!IDC() || (o == NULL) || (gbuf == 0))
	    return;
	/* As osw-x.cpp, the buffer also becomes the background. */
	OSWSetWindowBkg(w, 0, (pixmap_t)gbuf);
	OSWCopyDrawables((drawable_t)w, (drawable_t)gbuf, o->width, o->height);
}

void OSWCopyDrawablesCoord(
	drawable_t tar_d, drawable_t src_d,
	int tar_x, int tar_y,
	unsigned int width, unsigned int height,
	int src_x, int src_y
)
{
	obj *s = O(src_d);
	buffer b;

	if(!IDC() || (s == NULL) || (s->data == NULL) ||
	   (width == 0) || (height == 0) || !Target(tar_d, &b)
	)
	    return;
	Blit(&b, tar_x, tar_y, s->data, s->width, s->height, s->width * 4,
	    src_x, src_y, width, height);
}

void OSWCopyDrawables(
	drawable_t tar_d, drawable_t src_d,
	unsigned int width, unsigned int height
)
{
	OSWCopyDrawablesCoord(tar_d, src_d, 0, 0, width, height, 0, 0);
}


/* ******************************************************************
 *
 *	Drawing (with the foreground pixel and font, as X's GC).
 */
static inline u_int32_t Fg(void)
{
	return((u_int32_t)osw_gui[0].current_fg_pix);
}

/* Zero width line, both end points drawn (as X with CapButt). */
static void Line(buffer *b, int x0, int y0, int x1, int y1, u_int32_t c)
{
	int dx = abs(x1 - x0), dy = -abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
	int err = dx + dy, e2;

	while(1)
	{
	    PutPixel(b, x0, y0, c);
	    if((x0 == x1) && (y0 == y1))
		break;
	    e2 = 2 * err;
	    if(e2 >= dy) { err += dy; x0 += sx; }
	    if(e2 <= dx) { err += dx; y0 += sy; }
	}
}

void OSWDrawString(drawable_t d, int x, int y, const char *string)
{
	if(string == NULL)
	    return;
	OSWDrawStringLimited(d, x, y, string, strlen(string));
}

void OSWDrawStringLimited(drawable_t d, int x, int y, const char *string, int len)
{
	font_t *font = osw_gui[0].current_font;
	const OSWSDLFont *f;
	u_int32_t c = Fg();
	buffer b;
	int i, gx, gy;

	if(!IDC() || (string == NULL) || (len < 1) || !Target(d, &b))
	    return;
	if(font == NULL)
	    font = osw_gui[0].std_font;
	if((font == NULL) || (font->actual == NULL))
	    return;
	f = (const OSWSDLFont *)font->actual;

	/* x, y is the origin of the first character on the baseline. */
	for(i = 0; (i < len) && (string[i] != '\0'); i++)
	{
	    const OSWSDLGlyph *g = &f->glyph[(unsigned char)string[i]];
	    const unsigned short *rows = &f->rows[g->row_start];
	    int w = g->rsb - g->lsb, h = g->ascent + g->descent;

	    for(gy = 0; gy < h; gy++)
	    {
		for(gx = 0; gx < w; gx++)
		{
		    if(rows[gy] & (0x8000 >> gx))
			PutPixel(&b, x + g->lsb + gx, y - g->ascent + gy, c);
		}
	    }
	    x += g->width;
	}
}

void OSWDrawLine(drawable_t d, int start_x, int start_y, int end_x, int end_y)
{
	buffer b;

	if(!IDC() || !Target(d, &b))
	    return;
	Line(&b, start_x, start_y, end_x, end_y, Fg());
}

void OSWDrawRectangle(drawable_t d, int x, int y,
	unsigned int width, unsigned int height)
{
	buffer b;
	int x2 = x + (int)width, y2 = y + (int)height;
	u_int32_t c = Fg();

	if(!IDC() || (width == 0) || (height == 0) || !Target(d, &b))
	    return;
	/* X draws the outline of width + 1 by height + 1 pixels. */
	Line(&b, x, y, x2, y, c);
	Line(&b, x2, y, x2, y2, c);
	Line(&b, x2, y2, x, y2, c);
	Line(&b, x, y2, x, y, c);
}

void OSWDrawSolidRectangle(drawable_t d, int x, int y,
	unsigned int width, unsigned int height)
{
	buffer b;

	if(!IDC() || (width == 0) || (height == 0) || !Target(d, &b))
	    return;
	FillRect(&b, x, y, (int)width, (int)height, Fg());
}

/* True if angle a (radians) is in the arc from a1 extending a2. */
static bool InArc(double a, double a1, double a2)
{
	double t = (a2 < 0.0) ? -a2 : a2;

	if(t >= 2.0 * PI)
	    return(true);
	if(a2 < 0.0)
	    a1 += a2;
	a = fmod(a - a1, 2.0 * PI);
	if(a < 0.0)
	    a += 2.0 * PI;
	return(a <= t);
}

/*   Angles as X: in 64ths of a degree (the caller's radians are
 *   truncated the same way), counterclockwise from 3 o'clock.
 */
static void ArcAngles(double pos, double term, double *a1, double *a2)
{
	*a1 = (double)static_cast<int>(pos * 180 / PI * 64) / 64.0 * PI / 180.0;
	*a2 = (double)static_cast<int>(term * 180 / PI * 64) / 64.0 * PI / 180.0;
}

void OSWDrawArc(drawable_t d, int x, int y,
	unsigned int width, unsigned int height,
	double position_angle, double terminal_angle)
{
	buffer b;
	double a1, a2, rx = width / 2.0, ry = height / 2.0;
	double cx = x + rx, cy = y + ry, a, step;
	int i, n, px, py, lx = 0, ly = 0;
	u_int32_t c = Fg();

	if(!IDC() || (width == 0) || (height == 0) || !Target(d, &b))
	    return;
	ArcAngles(position_angle, terminal_angle, &a1, &a2);
	if(a2 > 2.0 * PI) a2 = 2.0 * PI;
	if(a2 < -2.0 * PI) a2 = -2.0 * PI;

	n = (int)(fabs(a2) * MAX(rx, ry)) + 8;
	step = a2 / n;
	for(i = 0; i <= n; i++)
	{
	    a = a1 + (step * i);
	    px = (int)floor(cx + (rx * cos(a)) + 0.5);
	    py = (int)floor(cy - (ry * sin(a)) + 0.5);
	    if(i == 0)
		PutPixel(&b, px, py, c);
	    else
		Line(&b, lx, ly, px, py, c);
	    lx = px;
	    ly = py;
	}
}

void OSWDrawSolidArc(drawable_t d, int x, int y,
	unsigned int width, unsigned int height,
	double position_angle, double terminal_angle)
{
	buffer b;
	double a1, a2, rx = width / 2.0, ry = height / 2.0;
	double cx = x + rx, cy = y + ry, dx, dy;
	int px, py;
	u_int32_t c = Fg();

	if(!IDC() || (width == 0) || (height == 0) || !Target(d, &b))
	    return;
	ArcAngles(position_angle, terminal_angle, &a1, &a2);

	/* Pie slice: pixel centers inside the ellipse and the angles. */
	for(py = y; py < y + (int)height; py++)
	{
	    for(px = x; px < x + (int)width; px++)
	    {
		dx = (px + 0.5 - cx) / rx;
		dy = (py + 0.5 - cy) / ry;
		if((dx * dx) + (dy * dy) > 1.0)
		    continue;
		if(!InArc(atan2(-dy, dx), a1, a2))
		    continue;
		PutPixel(&b, px, py, c);
	    }
	}
}
