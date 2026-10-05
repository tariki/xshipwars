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

	Sound is played with SDL2_mixer (SNDSERV_TYPE_SDL, needs
	HAVE_SDL_MIXER). The YIFF and EsounD sound servers this file used
	to talk to no longer exist, so their code has been removed.
	sound.server_type still accepts SNDSERV_TYPE_YIFF/ESOUND/MIKMOD
	from old configuration files, but they produce no sound.
 */

#ifdef HAVE_SDL_MIXER
# include <SDL.h>
# include <SDL_mixer.h>
#endif	/* HAVE_SDL_MIXER */

#include "xsw.h"
#include "ss.h"


#ifdef HAVE_SDL_MIXER
/* Mixer output format and number of sounds that can play at once. */
#define SOUND_SDL_FREQUENCY	44100
#define SOUND_SDL_CHANNELS	16

namespace static_sound_sdl {
	/* Marks sound.con_data as initialized for the SDL backend. */
	int initialized;

	/*   Loaded sound effects, by sound code. The path is kept so a
	 *   reloaded sound scheme that maps a code to another file is
	 *   picked up.
	 */
	struct cached_chunk {
	    int code;
	    char *path;
	    Mix_Chunk *chunk;
	};
	cached_chunk *cache;
	int total_cached;
}

/*
 *	Returns the loaded sound effect for code, loading it from the
 *	sound scheme's file as needed, or NULL on error.
 */
static Mix_Chunk *SoundSDLGetChunk(int code)
{
	int i;
	const char *path = ss_item[code]->path;
	static_sound_sdl::cached_chunk *c;

	if(path == NULL)
	    return(NULL);

	for(i = 0; i < static_sound_sdl::total_cached; i++)
	{
	    c = &static_sound_sdl::cache[i];
	    if(c->code != code)
		continue;
	    if((c->path != NULL) && !strcmp(c->path, path))
		return(c->chunk);

	    /* Scheme now maps this code to another file. */
	    Mix_FreeChunk(c->chunk);
	    free(c->path);
	    c->path = StringCopyAlloc(path);
	    c->chunk = Mix_LoadWAV(path);
	    if(c->chunk == NULL)
		fprintf(stderr, "%s: %s\n", path, Mix_GetError());
	    return(c->chunk);
	}

	c = (static_sound_sdl::cached_chunk *)realloc(
	    static_sound_sdl::cache,
	    (static_sound_sdl::total_cached + 1) * sizeof(*c)
	);
	if(c == NULL)
	    return(NULL);
	static_sound_sdl::cache = c;
	c = &static_sound_sdl::cache[static_sound_sdl::total_cached++];
	c->code = code;
	c->path = StringCopyAlloc(path);
	c->chunk = Mix_LoadWAV(path);
	if(c->chunk == NULL)
	    fprintf(stderr, "%s: %s\n", path, Mix_GetError());
	return(c->chunk);
}

/*
 *	Frees all loaded sound effects.
 */
static void SoundSDLFreeChunks(void)
{
	int i;

	for(i = 0; i < static_sound_sdl::total_cached; i++)
	{
	    Mix_FreeChunk(static_sound_sdl::cache[i].chunk);
	    free(static_sound_sdl::cache[i].path);
	}
	free(static_sound_sdl::cache);
	static_sound_sdl::cache = NULL;
	static_sound_sdl::total_cached = 0;
}

/*
 *	Converts a 0.0 to 1.0 volume to a Mix_SetPanning() level.
 */
static Uint8 SoundSDLLevel(double v)
{
	if(v < 0.0)
	    v = 0.0;
	if(v > 1.0)
	    v = 1.0;
	return((Uint8)(v * 255.0 + 0.5));
}
#endif	/* HAVE_SDL_MIXER */


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
#ifdef HAVE_SDL_MIXER
	  case SNDSERV_TYPE_SDL:
	    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	    {
		fprintf(stderr, "SoundInit(): SDL audio: %s\n", SDL_GetError());
		return(-1);
	    }
	    if(Mix_OpenAudio(
		SOUND_SDL_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024
	       ) < 0
	    )
	    {
		fprintf(stderr, "SoundInit(): SDL_mixer: %s\n", Mix_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return(-1);
	    }
	    Mix_AllocateChannels(SOUND_SDL_CHANNELS);
	    sound.con_data = (void *)&static_sound_sdl::initialized;
	    break;
#endif	/* HAVE_SDL_MIXER */

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
#ifdef HAVE_SDL_MIXER
	  case SNDSERV_TYPE_SDL:
	    if(1)
	    {
		int channel;
		Mix_Chunk *chunk = SoundSDLGetChunk(code);

		if(chunk == NULL)
		    return(-1);

		/*   Pick a free channel and set its volumes before
		 *   starting it, so the start is not heard at the
		 *   previous channel's volumes. Fails only when all
		 *   channels are busy.
		 */
		channel = Mix_GroupAvailable(-1);
		if(channel < 0)
		    return(-1);
		Mix_SetPanning(
		    channel,
		    SoundSDLLevel(left_volume),
		    SoundSDLLevel(right_volume)
		);
		if(Mix_PlayChannel(channel, chunk, 0) < 0)
		    return(-1);
	    }
	    break;
#endif	/* HAVE_SDL_MIXER */

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
#ifdef HAVE_SDL_MIXER
	  case SNDSERV_TYPE_SDL:
	    if(sound.con_data != NULL)
	    {
		Mix_HaltChannel(-1);
		SoundSDLFreeChunks();
		Mix_CloseAudio();
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	    }
	    break;
#endif	/* HAVE_SDL_MIXER */

	  default:
	    break;
        }

	/* Reset values just in case. */
	sound.bkg_playid = NULL;
	sound.con_data = NULL;

	return;
}
