// include/osw-sdl.h

/*
		Operating System Wrapper for SDL2

	Data types for the SDL2 implementation of the OSW layer
	(global/osw-sdl.cpp), read by osw-x.h when OSW_SDL is defined.

	The rest of the program was written for X and uses X's names
	directly: the event structure and its members (xany.window,
	xkey.keycode, xbutton.*, xmotion.*, xvisibility.state, ...),
	event types and masks, Button1, True/False/None and so on. They
	are defined here with the same names and values as X's, so that
	the callers do not change. X_H is NOT defined, so code that is
	only for X (#ifdef X_H) drops out.

	Windows, pixmaps and cursors are handles (small integers, 0 is
	None) into tables kept in osw-sdl.cpp.
 */

#ifndef OSW_SDL_H
#define OSW_SDL_H

#include <sys/types.h>

#include "os.h"


/* ******************************************************************
 *
 *                     X compatible constants
 */

#ifndef True
# define True		1
#endif
#ifndef False
# define False		0
#endif
#ifndef None
# define None		0L
#endif

typedef int Bool;
typedef int Status;
typedef unsigned long Time;

/* Event types. */
#define KeyPress		2
#define KeyRelease		3
#define ButtonPress		4
#define ButtonRelease		5
#define MotionNotify		6
#define EnterNotify		7
#define LeaveNotify		8
#define FocusIn			9
#define FocusOut		10
#define Expose			12
#define VisibilityNotify	15
#define DestroyNotify		17
#define UnmapNotify		18
#define MapNotify		19
#define ConfigureNotify		22
#define ClientMessage		33
#define LASTEvent		36

/* Event masks. */
#define NoEventMask		0L
#define KeyPressMask		(1L<<0)
#define KeyReleaseMask		(1L<<1)
#define ButtonPressMask		(1L<<2)
#define ButtonReleaseMask	(1L<<3)
#define EnterWindowMask		(1L<<4)
#define LeaveWindowMask		(1L<<5)
#define PointerMotionMask	(1L<<6)
#define PointerMotionHintMask	(1L<<7)
#define Button1MotionMask	(1L<<8)
#define Button2MotionMask	(1L<<9)
#define Button3MotionMask	(1L<<10)
#define Button4MotionMask	(1L<<11)
#define Button5MotionMask	(1L<<12)
#define ButtonMotionMask	(1L<<13)
#define KeymapStateMask		(1L<<14)
#define ExposureMask		(1L<<15)
#define VisibilityChangeMask	(1L<<16)
#define StructureNotifyMask	(1L<<17)
#define ResizeRedirectMask	(1L<<18)
#define SubstructureNotifyMask	(1L<<19)
#define SubstructureRedirectMask (1L<<20)
#define FocusChangeMask		(1L<<21)
#define PropertyChangeMask	(1L<<22)
#define ColormapChangeMask	(1L<<23)
#define OwnerGrabButtonMask	(1L<<24)

/* Key and button modifier masks. */
#define ShiftMask		(1<<0)
#define LockMask		(1<<1)
#define ControlMask		(1<<2)
#define Mod1Mask		(1<<3)
#define Button1Mask		(1<<8)
#define Button2Mask		(1<<9)
#define Button3Mask		(1<<10)
#define Button4Mask		(1<<11)
#define Button5Mask		(1<<12)

/* Buttons. */
#define Button1			1
#define Button2			2
#define Button3			3
#define Button4			4
#define Button5			5

/* Visibility states. */
#define VisibilityUnobscured		0
#define VisibilityPartiallyObscured	1
#define VisibilityFullyObscured		2

/* Map states (win_attr_t map_state). */
#define IsUnmapped		0
#define IsUnviewable		1
#define IsViewable		2

/* Pointer grab modes and results. */
#define GrabModeSync		0
#define GrabModeAsync		1
#define GrabSuccess		0
#define AlreadyGrabbed		1

/* Basic pointer cursors (codes from X's cursorfont.h). */
#define XC_X_cursor		0
#define XC_arrow		2
#define XC_crosshair		34
#define XC_fleur		52
#define XC_hand2		60
#define XC_left_ptr		68
#define XC_sb_h_double_arrow	108
#define XC_sb_v_double_arrow	116
#define XC_watch		150
#define XC_xterm		152


/* ******************************************************************
 *
 *                         Data Types
 */

/* Opaque types that only the X implementation looks into. */
typedef struct OSWSDLDisplay OSWSDLDisplay;
typedef struct OSWSDLScreen OSWSDLScreen;
typedef struct OSWSDLVisual OSWSDLVisual;

#define atom_t		unsigned long
#define bool_t		Bool
#define colormap_t	unsigned long
#define cursor_t	unsigned long
#define cur_code_t	unsigned int
#define depth_t		int
#define display_t	OSWSDLDisplay
#define drawable_t	unsigned long
#define eventmask_t	long
#define eventtype_t	int
#define gbuf_t		unsigned long
#define gc_t		void *
#define keycode_t	unsigned int
#define keysym_t	unsigned long
#define pixel_t		unsigned long
#define pixmap_t	unsigned long
#define screen_num_t	int
#define screen_ptr_t	OSWSDLScreen
#define visibility_t	int
#define visual_t	OSWSDLVisual
#define visual_id_t	unsigned long
#define visual_info_mask_t	long
#define win_t		unsigned long

typedef struct {
	int dummy;
} gc_val_t;

typedef struct {
	int dummy;
} visual_info_t;


/*
 *    Font structure:
 */
typedef struct {

	/* Can be 0 for non fixed sized fonts. */
	unsigned int char_width, char_height;

	/* Total number of different characters. */
	int total_chars;

	/* The built in bitmap font (private). */
	const void *actual;

} OSWFontStruct;
#define font_t		OSWFontStruct


/*
 *    Image structure (the members of X's XImage that are used):
 *
 *	Always ZPixmap, 24 bit depth in 32 bit pixels 0x00RRGGBB in
 *	the host byte order.
 */
typedef struct {

	int width, height;
	int xoffset;
	int format;
	char *data;
	int byte_order;
	int bitmap_pad;
	int depth;
	int bytes_per_line;
	int bits_per_pixel;
	unsigned long red_mask, green_mask, blue_mask;

} OSWImageStruct;
#define image_t		OSWImageStruct


/*
 *    Shared image structure:
 */
typedef struct {

	/* Header info. */
	char in_progress;	/* True if currently being `put'. */

	int byte_order;
	int xoffset;
	int format;
	u_int8_t *data;		/* Pointer to image data. */

	int bytes_per_line;	/* Accelarator to next line. */
	int bits_per_pixel;
	int bitmap_pad;

	unsigned int width, height;
	unsigned int depth;	/* Depth of image in bits. */

	unsigned long red_mask;
	unsigned long green_mask;
	unsigned long blue_mask;

	/* The image that holds data (PRIVATE!). */
	image_t *ximage;

	char shminfo;

} OSWSharedImageStruct;
#define shared_image_t	OSWSharedImageStruct


/*
 *    Size hints structure (X's XSizeHints):
 */
typedef struct {

	long flags;
	int x, y;
	int width, height;
	int min_width, min_height;
	int max_width, max_height;

} OSWSizeHintsStruct;
#define sizehints_t	OSWSizeHintsStruct


/*
 *    Window attributes structure (X's XWindowAttributes):
 */
typedef struct {

	int x, y;
	int width, height;
	int border_width;
	int depth;
	win_t root;
	int map_state;		/* IsUnmapped, IsUnviewable or IsViewable. */
	long your_event_mask;

} OSWWindowAttributesStruct;
#define win_attr_t	OSWWindowAttributesStruct


/*
 *    Event structures (the members of X's events that are used):
 */
typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t window;
} OSWAnyEvent;

typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t window;
	win_t root;
	win_t subwindow;
	Time time;
	int x, y;
	int x_root, y_root;
	unsigned int state;
	unsigned int keycode;
	bool_t same_screen;

	/* Character typed with this key press (SDL's text input),
	 * '\0' if none (private, for OSWGetASCIIFromKeyCode()).
	 */
	char text;
} OSWKeyEvent;
#define key_event_t	OSWKeyEvent

typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t window;
	win_t root;
	win_t subwindow;
	Time time;
	int x, y;
	int x_root, y_root;
	unsigned int state;
	unsigned int button;
	bool_t same_screen;
} OSWButtonEvent;

typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t window;
	win_t root;
	win_t subwindow;
	Time time;
	int x, y;
	int x_root, y_root;
	unsigned int state;
	char is_hint;
	bool_t same_screen;
} OSWMotionEvent;

typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t window;
	win_t root;
	win_t subwindow;
	Time time;
	int x, y;
	int x_root, y_root;
	int mode;
	int detail;
	bool_t same_screen;
	bool_t focus;
	unsigned int state;
} OSWCrossingEvent;

typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t window;
	int mode;
	int detail;
} OSWFocusEvent;

typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t window;
	int x, y;
	int width, height;
	int count;
} OSWExposeEvent;

typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t window;
	int state;
} OSWVisibilityEvent;

typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t event;
	win_t window;
	int x, y;
	int width, height;
	int border_width;
	win_t above;
	bool_t override_redirect;
} OSWConfigureEvent;

typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t event;
	win_t window;
} OSWStructureEvent;	/* MapNotify, UnmapNotify and DestroyNotify. */

typedef struct {
	int type;
	unsigned long serial;
	bool_t send_event;
	display_t *display;
	win_t window;
	atom_t message_type;
	int format;
	union {
		char b[20];
		short s[10];
		long l[5];
	} data;
} OSWClientMessageEvent;

typedef union {
	int type;
	OSWAnyEvent xany;
	OSWKeyEvent xkey;
	OSWButtonEvent xbutton;
	OSWMotionEvent xmotion;
	OSWCrossingEvent xcrossing;
	OSWFocusEvent xfocus;
	OSWExposeEvent xexpose;
	OSWVisibilityEvent xvisibility;
	OSWConfigureEvent xconfigure;
	OSWStructureEvent xmap;
	OSWStructureEvent xunmap;
	OSWStructureEvent xdestroywindow;
	OSWClientMessageEvent xclient;
	long pad[24];
} OSWEvent;
#define event_t		OSWEvent


/* Definitions and functions common to all GUI implementations. */
#include "osw-api.h"


#endif /* OSW_SDL_H */
