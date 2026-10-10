// global/osw-sdl.cpp
/*
		Operating System Wrapper for SDL2

	The SDL2 implementation of the OSW layer, used instead of
	osw-x.cpp when built with GUI=sdl (OSW_SDL defined). See
	include/osw-sdl.h for the data types.

	Under construction: these are stubs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/osw-x.h"
#include "../include/graphics.h"


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


int OSWGUIConnect(int argc, char *argv[])
{
	fprintf(stderr, "OSWGUIConnect(): SDL2 GUI is not implemented yet.\n");
	return(-1);
}

void OSWGUIDisconnect(void)
{
	return;
}

int OSWLoadKeyCodes(void)
{
	return(-1);
}

char OSWGetASCIIFromKeyCode(key_event_t *ke, bool_t shift, bool_t alt, bool_t ctrl)
{
	return(0);
}

const char * OSWGetKeyCodeName(keycode_t keycode)
{
	return(NULL);
}

int OSWIsModifierKey(keycode_t keycode)
{
	return(-1);
}

void OSWKBAutoRepeatOff(void)
{
	return;
}

void OSWKBAutoRepeatOn(void)
{
	return;
}

void OSWGUISync(bool_t discard)
{
	return;
}

void OSWGetPointerCoords(win_t w, int *root_x, int *root_y, int *wx, int *wy)
{
	return;
}

int OSWGrabPointer(win_t grab_w, bool_t events_rel_grab_w, /* Relative to grab_w if True. */ eventmask_t eventmask, int pointer_mode, /* GrabModeSync or GrabModeAsync. */ int keyboard_mode, /* GrabModeSync or GrabModeAsync. */ win_t confine_w, /* Can be None. */ cursor_t cursor /* Can be None. */)
{
	return(-1);
}

void OSWUngrabPointer(void)
{
	return;
}

void OSWGUIFree(void **ptr)
{
	return;
}

void * OSWFetchDDE(int *bytes)
{
	return(NULL);
}

void OSWPutDDE(void *buf, int bytes)
{
	return;
}

visual_t * OSWGetVisualByCriteria(visual_info_mask_t vinfo_mask, visual_info_t criteria_vinfo)
{
	return(NULL);
}

visual_t * OSWGetVisualByID(visual_id_t vid)
{
	return(NULL);
}

int OSWLoadFont(font_t **font, const char *fontname)
{
	return(-1);
}

font_t * OSWQueryCurrentFont(void)
{
	return(NULL);
}

void OSWSetFont(font_t *font)
{
	return;
}

void OSWUnloadFont(font_t **font)
{
	return;
}

int OSWLoadPixelRGB(pixel_t *pix_rtn, u_int8_t r, u_int8_t g, u_int8_t b)
{
	return(-1);
}

int OSWLoadPixelHSL(pixel_t *pix_rtn, u_int8_t h, u_int8_t s, u_int8_t l)
{
	return(-1);
}

int OSWLoadPixelCLSP(pixel_t *pix_rtn, const char *clsp)
{
	return(-1);
}

void OSWDestroyPixel(pixel_t *pix_ptr)
{
	return;
}

void OSWSetFgPix(pixel_t pix)
{
	return;
}

cursor_t OSWLoadBasicCursor(cur_code_t code)
{
	return(0);
}

void OSWSetWindowCursor(win_t w, cursor_t cursor)
{
	return;
}

void OSWUnsetWindowCursor(win_t w)
{
	return;
}

void OSWDestroyCursor(cursor_t *cursor)
{
	return;
}

cursor_t OSWCreateCursorFromXpmFile(char *xpmfile, int *hot_x, int *hot_y, u_int8_t r, u_int8_t g, u_int8_t b, unsigned int *width, unsigned int *height)
{
	return(0);
}

cursor_t OSWCreateCursorFromXpmData(const char **xpmdata, int *hot_x, int *hot_y, u_int8_t r, u_int8_t g, u_int8_t b, unsigned int *width, unsigned int *height)
{
	return(0);
}

cursor_t OSWCreateCursorFromImage(image_t *image, unsigned int *width_rtn, unsigned int *height_rtn)
{
	return(0);
}

int OSWEventsPending(void)
{
	return(-1);
}

void OSWWaitNextEvent(event_t *event)
{
	return;
}

void OSWWaitPeakEvent(event_t *event)
{
	return;
}

bool_t OSWCheckMaskEvent(eventmask_t eventmask, event_t *event)
{
	return(0);
}

void OSWWaitWindowEvent(win_t w, eventmask_t event_mask, event_t *event)
{
	return;
}

bool_t OSWCheckWindowEvent(win_t w, eventmask_t event_mask, event_t *event)
{
	return(0);
}

void OSWPutBackEvent(event_t *event)
{
	return;
}

int OSWSendEvent(eventmask_t mask, event_t *event, bool_t propagate)
{
	return(-1);
}

int OSWPurgeAllEvents(void)
{
	return(-1);
}

int OSWPurgeOldMotionEvents(void)
{
	return(-1);
}

int OSWPurgeTypedEvent(eventtype_t event_type)
{
	return(-1);
}

int OSWPurgeWindowTypedEvent(win_t w, eventtype_t event_type)
{
	return(-1);
}

int OSWIsEventDestroyWindow(win_t w, event_t *event)
{
	return(-1);
}

int OSWCreateWindow(win_t *w, win_t parent, int x, int y, unsigned int width, unsigned int height)
{
	return(-1);
}

int OSWCreateInputWindow(win_t *w, win_t parent, int x, int y, unsigned int width, unsigned int height)
{
	return(-1);
}

void OSWDestroyWindow(win_t *w)
{
	return;
}

bool_t OSWDrawableIsWindow(drawable_t d)
{
	return(0);
}

void OSWSetWindowWMProperties(win_t w, const char *title, const char *icon_title, pixmap_t icon, bool_t wm_sets_coordinates, int x, int y, unsigned int min_width, unsigned int min_height, unsigned int max_width, unsigned int max_height, int frame_style, char **argv, int argc)
{
	return;
}

void OSWSetWindowInput(win_t w, eventmask_t eventmask)
{
	return;
}

void OSWSetTransientFor(win_t wbum, win_t wshelter)
{
	return;
}

void OSWSetMainWindow(win_t w)
{
	return;
}

win_t OSWGetMainWindow(void)
{
	return(0);
}

void OSWMapWindow(win_t w)
{
	return;
}

void OSWMapRaised(win_t w)
{
	return;
}

void OSWMapSubwindows(win_t w)
{
	return;
}

void OSWUnmapWindow(win_t w)
{
	return;
}

void OSWRestackWindows(win_t *w, int num_w)
{
	return;
}

int OSWIconifyWindow(win_t w)
{
	return(-1);
}

void OSWReparentWindow(win_t w, win_t parent)
{
	return;
}

void OSWMoveWindow(win_t w, int x, int y)
{
	return;
}

void OSWResizeWindow(win_t w, unsigned int width, unsigned int height)
{
	return;
}

void OSWMoveResizeWindow(win_t w, int x, int y, unsigned int width, unsigned int height)
{
	return;
}

void OSWSetWindowTitle(win_t w, const char *title)
{
	return;
}

void OSWClearWindow(win_t w)
{
	return;
}

void OSWSetWindowBkg(win_t w, pixel_t pix, pixmap_t pixmap)
{
	return;
}

bool_t OSWGetWindowAttributes(win_t w, win_attr_t *wattr)
{
	return(0);
}

int OSWGetWindowRootPos(win_t w, int *x, int *y)
{
	return(-1);
}

win_t OSWGetWindowParent(win_t w)
{
	return(0);
}

bool_t OSWCheckWindowAncestory(win_t grand_parent, win_t grand_child)
{
	return(0);
}

int OSWCreatePixmap(pixmap_t *pixmap, unsigned int width, unsigned int height)
{
	return(-1);
}

void OSWDestroyPixmap(pixmap_t *pixmap)
{
	return;
}

bool_t OSWDrawableIsPixmap(drawable_t d)
{
	return(0);
}

bool_t OSWGetPixmapAttributes(pixmap_t pixmap, pixmap_attr_t *pattr)
{
	return(0);
}

void OSWClearPixmap(pixmap_t pixmap, unsigned int width, unsigned int height, pixel_t pix)
{
	return;
}

int OSWCreateImage(image_t **image, unsigned int width, unsigned int height)
{
	return(-1);
}

void OSWDestroyImage(image_t **image)
{
	return;
}

int OSWCreateSharedImage(shared_image_t **image, unsigned int width, unsigned int height)
{
	return(-1);
}

void OSWDestroySharedImage(shared_image_t **image)
{
	return;
}

pixmap_t OSWCreatePixmapFromImage(image_t *image)
{
	return(0);
}

pixmap_t OSWCreatePixmapMaskFromImage(image_t *image)
{
	return(0);
}

image_t * OSWLoadImageFromXpmFile(char *filename)
{
	return(NULL);
}

image_t * OSWLoadImageFromXpmData(char **data)
{
	return(NULL);
}

pixmap_t OSWLoadPixmapFromXpmFile(char *filename)
{
	return(0);
}

pixmap_t OSWLoadPixmapFromXpmData(char **data)
{
	return(0);
}

image_t * OSWGetImage(drawable_t d, int x, int y, unsigned int width, unsigned int height)
{
	return(NULL);
}

void OSWPutImageToDrawable(image_t *image, drawable_t d)
{
	return;
}

void OSWPutImageToDrawablePos(image_t *image, drawable_t d, int tar_x, int tar_y)
{
	return;
}

void OSWPutImageToDrawableSect(image_t *image, drawable_t d, int tar_x, int tar_y, int src_x, int src_y, unsigned int width, unsigned int height)
{
	return;
}

void OSWPutSharedImageToDrawable(shared_image_t *image, drawable_t d)
{
	return;
}

void OSWPutSharedImageToDrawablePos(shared_image_t *image, drawable_t d, int tar_x, int tar_y)
{
	return;
}

void OSWPutSharedImageToDrawableSect(shared_image_t *image, drawable_t d, int tar_x, int tar_y, int src_x, int src_y, unsigned int width, unsigned int height)
{
	return;
}

void OSWSyncSharedImage(shared_image_t *image, drawable_t d)
{
	return;
}

void OSWPutBufferToWindow(win_t w, gbuf_t gbuf)
{
	return;
}

void OSWCopyDrawables(drawable_t tar_d, drawable_t src_d, unsigned int width, unsigned int height)
{
	return;
}

void OSWCopyDrawablesCoord(drawable_t tar_d, drawable_t src_d, int tar_x, int tar_y, unsigned int width, unsigned int height, int src_x, int src_y)
{
	return;
}

void OSWDrawString(drawable_t d, int x, int y, const char *string)
{
	return;
}

void OSWDrawStringLimited(drawable_t d, int x, int y, const char *string, int len)
{
	return;
}

void OSWDrawLine(drawable_t d, int start_x, int start_y, int end_x, int end_y)
{
	return;
}

void OSWDrawRectangle(drawable_t d, int x, int y, unsigned int width, unsigned int height)
{
	return;
}

void OSWDrawSolidRectangle(drawable_t d, int x, int y, unsigned int width, unsigned int height)
{
	return;
}

void OSWDrawArc(drawable_t d, int x, int y, unsigned int width, unsigned int height, double position_angle, double terminal_angle)
{
	return;
}

void OSWDrawSolidArc(drawable_t d, int x, int y, unsigned int width, unsigned int height, double position_angle, double terminal_angle)
{
	return;
}
