// include/osw-x.h

/*
                Operating System Wrapper for

                      X Window Systems


	Functions here are incomplete, working on it.

	                             --WolfPack

 */

#ifndef OSW_X_H
#define OSW_X_H

/* The SDL2 implementation of the OSW layer (built with GUI=sdl). */
#ifdef OSW_SDL
# include "osw-sdl.h"
#else

#include <sys/types.h>

/* Shared memory. */
#ifdef USE_XSHM
# include <sys/shm.h>
# include <sys/ipc.h>
#endif /* USE_XSHM */


/* X */
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

/* Xext (X Extensions) */
#include <X11/extensions/shape.h>

/* MIT Shared Memory Extension. */
#ifdef USE_XSHM
# include <X11/extensions/XShm.h>
#endif /* USE_XSHM */

/* Catch any undefined stuff (mostly for Solaris). */
#include "os.h"







/* *****************************************************************
 *
 *                        OSW Structures
 */

/*
 *    Font structure:
 */
typedef struct {

	/* Can be 0 for non fixed sized fonts. */
	unsigned int char_width, char_height;

	/* Total number of different characters. */
	int total_chars;

	/* Actual GUI's font type structure (private). */
	XFontStruct *actual;

} OSWFontStruct;


/*
 *    Shared image structure:
 */
typedef struct {

	/* Header info. */
	char in_progress;	/* True if currently being `put'. */

	int byte_order;		/* LSBFirst or MSBFirst. */
	int xoffset;		/* Num of pixels offset in X direction. */
	int format;		/* XYBitmap, XYPixmap, or ZPixmap. */
	u_int8_t *data;		/* Pointer to image data. */

	int bytes_per_line;	/* Accelarator to next line. */
	int bits_per_pixel;	/* Bits per pixel (for ZPixmap). */
	int bitmap_pad;		/* 8, 16, 32 either XY or ZPixmap */

	unsigned int width, height;
	unsigned int depth;	/* Depth of image in bits. */

	unsigned long red_mask;		/* Bits in Z arrangment. */
	unsigned long green_mask;
	unsigned long blue_mask;


	/* Actual ximage, native to GUI (PRIVATE!). */
	XImage *ximage;

	/* Shared memory info, native to GUI (PRIVATE!). */
#ifdef USE_XSHM
	XShmSegmentInfo	shminfo;
#else
	char		shminfo;
#endif

} OSWSharedImageStruct;
  



/* *****************************************************************
 *
 *                         Data Types
 */

/*
 *    Atom type:
 */
#define atom_t		Atom

/*
 *    Boolean type:
 */
#define bool_t		Bool

/*
 *    Color map type:
 */
#define colormap_t	Colormap

/*
 *    Cursor type:
 */
#define cursor_t	Cursor

/*
 *    Cursor code type:
 */
#define cur_code_t	unsigned int

/*
 *	Depth type:
 */
#define depth_t		int

/*
 *    Display pointer type:
 */
#define display_t	Display

/*
 *    Drawable type (win_t and pixmap_t):
 */
#define drawable_t	Drawable

/*
 *    Key event type:
 */
#define key_event_t	XKeyEvent

/*
 *    Event type:
 */
#define event_t		XEvent

/*
 *    Event mask type:
 */
#define eventmask_t	long

/*
 *    Event type type:
 */
#define eventtype_t	int

/*
 *    Font structure type:
 */
#define font_t		OSWFontStruct

/*
 *    Graphics buffer type (same as pixmap_t):
 */
#define gbuf_t		Pixmap

/*
 *    Graphics context type:
 */
#define gc_t		GC

/*
 *    Graphics context value type:
 */
#define gc_val_t	XGCValues

/*
 *    Image type:
 */
#define image_t		XImage

/*
 *    Key code (actual hardware code) type:
 */
#define keycode_t	unsigned int

/*
 *    Key sym code (portable key defination code) type:
 */
#define keysym_t	KeySym

/*
 *    Pixel type:
 */
#define pixel_t		unsigned long  

/*
 *    Pixmap type:
 */
#define pixmap_t	Pixmap

/*
 *    Screen number type:
 */
#define screen_num_t	int

/*
 *    Screen pointer type:
 */
#define screen_ptr_t	Screen

/*
 *    Shared image type
 */
#define shared_image_t	OSWSharedImageStruct

/* 
 *    Size hints structure type:
 */
#define sizehints_t	XSizeHints

/*
 *    Visiblity code type:
 */
#define visibility_t	int

/*
 *    Visual type:
 */
#define visual_t	Visual

/*
 *    Visual info type:
 */
#define visual_info_t	XVisualInfo

/*
 *    Visual info mask type:
 */
#define visual_info_mask_t	long

/*
 *    Visual ID code type:
 */
#define visual_id_t	VisualID   

/*
 *    Window type:
 */
#define win_t		Window

/*
 *    Window attributes structure type:
 */
#define win_attr_t	XWindowAttributes






#ifdef OSW_USE_NATIVE_EVENT_TYPE
/*
 *      Native event type, under construction, do NOT compile in!
 */

typedef struct {

	int type;
	unsigned long serial;	/* # of last request processed by server */
	bool_t send_event;	/* true if this came from a SendEvent */

	display_t *display;	/* Display the event was read from */
	win_t window;

} any_event_t

typedef struct {

	int type;		/* KeyPress or KeyRelease */
	unsigned long serial;	/* # of last request processed by server */
	bool_t send_event;	/* True if this came from a SendEvent request */
	display_t *display;	/* Display the event was read from */
        win_t window;		/* Event window it is reported relative to */
	win_t root;		/* Root window that the event occurred on */
	win_t subwindow;	/* child window */

	Time time;		/* In milliseconds */
	int x, y;		/* Pointer x, y coordinates in event window */
	int x_root, y_root;	/* Coordinates relative to root */
	unsigned int state;	/* Key or button mask */
	unsigned int keycode;	/* Key code of the key in question. */
	bool_t same_screen;	/* Same screen flag. */
} key_event_t;


typedef struct {
	int type;		/* ButtonPress or ButtonRelease */
	unsigned long serial;	/* # of last request processed by server */
	bool_t send_event;	/* true if this came from a SendEvent request */
	display_t *display;	/* Display the event was read from */
	win_t window;		/* ``event'' window it is reported relative to */
	win_t root;		/* root window that the event occurred on */
	win_t subwindow;	/* child window */

	Time time;		/* In milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
	unsigned int state;	/* key or button mask */
	unsigned int button;	/* detail */
	bool_t same_screen;	/* same screen flag */
} button_event_t;






typedef struct {

	any_event_t	any;
	key_event_t	key;
	button_event_t	button;

} event_t;

#endif



/* Definitions and functions common to all GUI implementations. */
#include "osw-api.h"



#endif /* not OSW_SDL */

#endif /* OSW_X_H */
