/*
                    Joystick Functions on SDL2 (libjsw subset)

	Functions:

	int JSInit(
		js_data_struct *jsd,
		const char *device,
		const char *calibration,
		unsigned int flags
	)
	int JSUpdate(js_data_struct *jsd)
	double JSGetAxisCoeff(js_data_struct *jsd, int n)
	double JSGetAxisCoeffNZ(js_data_struct *jsd, int n)
	void JSClose(js_data_struct *jsd)

	---

	The joystick code was written for libjsw, which is no longer
	available. These are the libjsw functions the client uses,
	implemented with SDL2's joystick API so they work wherever SDL2
	finds joysticks (Linux and FreeBSD).

	The device name selects the SDL joystick by the number at its
	end, so the "/dev/js0" or "/dev/input/js1" of old configurations
	(and a plain "0") still work. libjsw calibration files are not
	read: axes use SDL's full range with libjsw's default null zone.
	Hats follow the axes as two axes each (x then y), as the Linux
	joystick driver that libjsw used presented them.
 */

#ifdef JS_SUPPORT

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <SDL.h>

#include "../include/jsw.h"


/* Raw range of SDL axes (hats are mapped to its ends). */
#define JS_SDL_AXIS_MIN		-32768
#define JS_SDL_AXIS_MAX		32767

/*   Null zone as a fraction of the half range, from libjsw's defaults
 *   for an uncalibrated axis (100 of 500).
 */
#define JS_SDL_NULL_ZONE	((double)JSDefaultNullZone / \
				 (JSDefaultMax - JSDefaultCenter))


/*
 *	Returns the SDL joystick index given by the number at the end of
 *	the device name, or -1 if it has none.
 */
static int JSSDLDeviceIndex(const char *device)
{
	const char *s;

	if(device == NULL)
	    device = JSDefaultDevice;

	s = device + strlen(device);
	while((s > device) && isdigit((unsigned char)s[-1]))
	    s--;
	if(*s == '\0')
	    return(-1);

	return(atoi(s));
}

/*
 *	Returns the open SDL joystick of jsd, or NULL.
 */
static SDL_Joystick *JSSDLJoystick(js_data_struct *jsd)
{
	if((jsd == NULL) || !(jsd->flags & JSFlagIsInit))
	    return(NULL);

	return(SDL_JoystickFromInstanceID((SDL_JoystickID)jsd->fd));
}

/*
 *	Returns the raw value of hat axis n (0 = x, 1 = y) for hat state.
 */
static int JSSDLHatValue(Uint8 state, int n)
{
	if(n == 0)
	{
	    if(state & SDL_HAT_LEFT)
		return(JS_SDL_AXIS_MIN);
	    if(state & SDL_HAT_RIGHT)
		return(JS_SDL_AXIS_MAX);
	}
	else
	{
	    if(state & SDL_HAT_UP)
		return(JS_SDL_AXIS_MIN);
	    if(state & SDL_HAT_DOWN)
		return(JS_SDL_AXIS_MAX);
	}
	return(0);
}


/*
 *	Opens the joystick given by device (see above) and sets up jsd
 *	with its axes and buttons. The calibration file is only recorded.
 *
 *	Returns JSSuccess or an error code. jsd may be passed to
 *	JSClose() in either case.
 */
int JSInit(
	js_data_struct *jsd,
	const char *device,
	const char *calibration,
	unsigned int flags
)
{
	int i, index, total_axes, total_hats;
	SDL_Joystick *js;
	const char *name;

	if(jsd == NULL)
	    return(JSBadValue);

	memset(jsd, 0x00, sizeof(js_data_struct));
	jsd->fd = -1;

	if(device == NULL)
	    device = JSDefaultDevice;
	index = JSSDLDeviceIndex(device);
	if(index < 0)
	    return(JSBadValue);

	/*   The client handles its own signals, and reads joysticks by
	 *   polling, so SDL's event queue is not used.
	 */
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
	    return(JSError);
	SDL_JoystickEventState(SDL_IGNORE);

	if(index >= SDL_NumJoysticks())
	{
	    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	    return(JSNoAccess);
	}
	js = SDL_JoystickOpen(index);
	if(js == NULL)
	{
	    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	    return(JSNoAccess);
	}

	jsd->fd = (int)SDL_JoystickInstanceID(js);
	jsd->flags = JSFlagIsInit | flags;

	name = SDL_JoystickName(js);
	jsd->name = strdup((name != NULL) ? name : "");
	jsd->device_name = strdup(device);
	jsd->calibration_file = (calibration != NULL) ?
	    strdup(calibration) : NULL;

	/* Axes, then two axes for each hat. */
	total_axes = SDL_JoystickNumAxes(js);
	total_hats = SDL_JoystickNumHats(js);
	if(total_axes < 0)
	    total_axes = 0;
	if(total_hats < 0)
	    total_hats = 0;
	jsd->total_axises = total_axes + (2 * total_hats);
	jsd->axis = (js_axis_struct **)calloc(
	    (jsd->total_axises > 0) ? jsd->total_axises : 1,
	    sizeof(js_axis_struct *)
	);
	for(i = 0; i < jsd->total_axises; i++)
	{
	    js_axis_struct *axis = (js_axis_struct *)calloc(
		1, sizeof(js_axis_struct)
	    );

	    jsd->axis[i] = axis;
	    if(axis == NULL)
		continue;
	    axis->min = JS_SDL_AXIS_MIN;
	    axis->cen = 0;
	    axis->max = JS_SDL_AXIS_MAX;
	    axis->nz = (int)(JS_SDL_NULL_ZONE * JS_SDL_AXIS_MAX);
	    if(i >= total_axes)
		axis->flags = JSAxisFlagIsHat;
	}

	jsd->total_buttons = SDL_JoystickNumButtons(js);
	if(jsd->total_buttons < 0)
	    jsd->total_buttons = 0;
	jsd->button = (js_button_struct **)calloc(
	    (jsd->total_buttons > 0) ? jsd->total_buttons : 1,
	    sizeof(js_button_struct *)
	);
	for(i = 0; i < jsd->total_buttons; i++)
	    jsd->button[i] = (js_button_struct *)calloc(
		1, sizeof(js_button_struct)
	    );

	/* Start from the current positions. */
	JSUpdate(jsd);

	return(JSSuccess);
}

/*
 *	Reads the joystick's current state into jsd.
 *
 *	Returns JSGotEvent if any axis or button changed since the last
 *	call, JSNoEvent otherwise (also when the joystick was unplugged).
 */
int JSUpdate(js_data_struct *jsd)
{
	int i, value, total_axes, got_event = 0;
	time_t t;
	SDL_Joystick *js = JSSDLJoystick(jsd);

	if(js == NULL)
	    return(JSNoEvent);

	SDL_JoystickUpdate();
	if(!SDL_JoystickGetAttached(js))
	    return(JSNoEvent);

	t = (time_t)SDL_GetTicks();
	total_axes = SDL_JoystickNumAxes(js);

	for(i = 0; i < jsd->total_axises; i++)
	{
	    js_axis_struct *axis = jsd->axis[i];

	    if(axis == NULL)
		continue;

	    if(i < total_axes)
		value = SDL_JoystickGetAxis(js, i);
	    else
		value = JSSDLHatValue(
		    SDL_JoystickGetHat(js, (i - total_axes) / 2),
		    (i - total_axes) % 2
		);
	    if(value == axis->cur)
		continue;

	    axis->prev = axis->cur;
	    axis->cur = value;
	    axis->last_time = axis->time;
	    axis->time = t;
	    got_event = 1;
	}

	for(i = 0; i < jsd->total_buttons; i++)
	{
	    js_button_struct *button = jsd->button[i];

	    if(button == NULL)
		continue;

	    value = SDL_JoystickGetButton(js, i) ?
		JSButtonStateOn : JSButtonStateOff;
	    button->prev_state = button->state;
	    if(value == button->state)
	    {
		button->changed_state = JSButtonChangedStateNone;
		continue;
	    }

	    button->state = value;
	    button->changed_state = (value == JSButtonStateOn) ?
		JSButtonChangedStateOffToOn : JSButtonChangedStateOnToOff;
	    button->last_time = button->time;
	    button->time = t;
	    got_event = 1;
	}

	return(got_event ? JSGotEvent : JSNoEvent);
}

/*
 *	Returns the position of axis n from -1.0 to 1.0, 0.0 at center.
 */
double JSGetAxisCoeff(js_data_struct *jsd, int n)
{
	double coeff;
	js_axis_struct *axis;

	if((jsd == NULL) || (n < 0) || (n >= jsd->total_axises))
	    return(0.0);
	axis = jsd->axis[n];
	if(axis == NULL)
	    return(0.0);

	if(axis->cur > axis->cen)
	    coeff = (double)(axis->cur - axis->cen) /
		(double)(axis->max - axis->cen);
	else
	    coeff = (double)(axis->cur - axis->cen) /
		(double)(axis->cen - axis->min);
	if(coeff > 1.0)
	    coeff = 1.0;
	else if(coeff < -1.0)
	    coeff = -1.0;

	return((axis->flags & JSAxisFlagFlipped) ? -coeff : coeff);
}

/*
 *	Same as JSGetAxisCoeff(), except that positions within the null
 *	zone around the center are 0.0 and the rest is scaled to reach
 *	-1.0 and 1.0 at the ends.
 */
double JSGetAxisCoeffNZ(js_data_struct *jsd, int n)
{
	double coeff, nz;
	js_axis_struct *axis;

	if((jsd == NULL) || (n < 0) || (n >= jsd->total_axises))
	    return(0.0);
	axis = jsd->axis[n];
	if(axis == NULL)
	    return(0.0);

	coeff = JSGetAxisCoeff(jsd, n);
	nz = (double)axis->nz / (double)(axis->max - axis->cen);
	if((nz <= 0.0) || (nz >= 1.0))
	    return(coeff);
	if(coeff > nz)
	    return((coeff - nz) / (1.0 - nz));
	if(coeff < -nz)
	    return((coeff + nz) / (1.0 - nz));
	return(0.0);
}

/*
 *	Closes the joystick and frees what JSInit() allocated in jsd.
 *	Does nothing if jsd was not initialized.
 */
void JSClose(js_data_struct *jsd)
{
	int i;
	SDL_Joystick *js;

	if(jsd == NULL)
	    return;
	if(!(jsd->flags & JSFlagIsInit))
	{
	    jsd->fd = -1;
	    return;
	}

	js = JSSDLJoystick(jsd);
	if(js != NULL)
	    SDL_JoystickClose(js);
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

	for(i = 0; i < jsd->total_axises; i++)
	    free(jsd->axis[i]);
	free(jsd->axis);
	for(i = 0; i < jsd->total_buttons; i++)
	    free(jsd->button[i]);
	free(jsd->button);
	free(jsd->name);
	free(jsd->device_name);
	free(jsd->calibration_file);

	memset(jsd, 0x00, sizeof(js_data_struct));
	jsd->fd = -1;
}

#endif	/* JS_SUPPORT */
