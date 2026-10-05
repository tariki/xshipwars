/*
                       Sound Server Communications

	Functions:

	int SoundInit()
	int SoundChangeMode(char *arg)
	int SoundPlay(
		int code,
		double left_volume,
		double right_volume,
		int effects,
		int priority
	)
	int SoundChangeBackgroundMusic(
	        int code,
	        int effects,
	        int priority
	)
	int SoundStopBackgroundMusic()
	int SoundManageEvents()
	void SoundShutdown()

	---

	The YIFF and EsounD sound servers this file used to talk to no
	longer exist, so their code has been removed. sound.server_type
	still accepts SNDSERV_TYPE_YIFF/ESOUND/MIKMOD from old
	configuration files, but they produce no sound.
 */

#include "xsw.h"
#include "ss.h"


/*
 *	Initializes the sound server, returns -1 on error.
 *
 *	Will return 0 and not initialize sound if option.sounds
 *	is XSW_SOUNDS_NONE.
 *
 *	sound.con_data must be NULL before calling this function!
 */
int SoundInit()
{
	/* Initialize by which sound server type: */
	switch(sound.server_type)
	{
	  default:
	    break;
	}

	/* Reset values. */
	sound.bkg_playid = NULL;
	sound.bkg_mood_code = -1;

	/* Load sound schemes from file. */
	SSLoadFromFile(fname.sound_scheme);

	return(0);
}

/*
 *	Changes the Audio mode to the one specified in arg. Does not
 *	change global sound.audio_mode_name.
 *
 *	Sound server should already be initialized or else no operation
 *	is performed.
 */
int SoundChangeMode(char *arg)
{
	/* Audio modes were a YIFF feature; nothing to change. */
	return(0);
}


/*
 *	Plays a sound by given code number.
 */
int SoundPlay(
        int code,
        double left_volume,	/* 0.0 to 1.0 */
        double right_volume,	/* 0.0 to 1.0 */
        int effects,
        int priority		/* 0 or 1. */
)
{
	if(sound.con_data == NULL)
	    return(-3);

	/* Get filename by sound code. */
	if(!SSIsAllocated(code))
	    return(-1);

        /* Play by which sound server type: */
        switch(sound.server_type)
        {
	  default:
	    break;
        }

	return(0);
}


/*
 *	Kills previous background mood music playback (if any)
 *	and then starts a new one specified by the sound code.
 *
 *	Globals sound.bkg_playid and sound.bkg_mood_code will be
 *	changed after a call to this function.
 */
int SoundChangeBackgroundMusic(
	int code,
        int effects,
        int priority            /* 0 or 1. */
)
{
        if(sound.con_data == NULL)
            return(-3);

        /* Play by which sound server type: */
        switch(sound.server_type)
        {
	  default:
	    break;
        }

	/* Update background mood code. */
	sound.bkg_mood_code = code;

        return(0);
}

int SoundStopBackgroundMusic()
{
        if(sound.con_data == NULL)
            return(-3);

	switch(sound.server_type)
	{
	  default:
	    break;
        }

        sound.bkg_mood_code = -1;
	sound.bkg_playid = NULL;

	return(0);
}

/*
 *	Manages sound events if any, does not block.
 *
 *	Returns number of events handled.
 */
int SoundManageEvents()
{
	return(0);
}


/*
 *	Shuts down the sound server.
 */
void SoundShutdown()
{
	/* Delete all sound schemes. */
	SSDeleteAll();

	/* Disconnect from sound server as needed. */
        switch(sound.server_type)
        {
	  default:
	    break;
        }

	/* Reset values just in case. */
	sound.bkg_playid = NULL;
	sound.con_data = NULL;

	return;
}
