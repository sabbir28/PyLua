/*
** PyLua Engine - Audio Subsystem
** Provides sound effects and music playback for the 2D game engine.
** Supports play/stop/pause/resume, volume, and looping.
** Backend: miniaudio
*/

#define engine_audio_c
#define LUA_LIB

#include "../../../../include/lprefix.h"

#include <stdio.h>
#include <string.h>

#include "../../../../include/lua.h"
#include "../../../../include/lauxlib.h"
#include "Engine_Audio.h"

/* 
** IMPORTANT: User must provide miniaudio.h in the include path or this directory.
** miniaudio: https://github.com/mackron/miniaudio
*/
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

/* ========================================================================= */
/* Internal State                                                            */
/* ========================================================================= */

#define MAX_AUDIO_CHANNELS 32

typedef struct {
    ma_sound sound;
    int playing;
} AudioChannel;

static struct {
    ma_engine engine;
    AudioChannel channels[MAX_AUDIO_CHANNELS];
    int initialized;
} audio_state;

/* ========================================================================= */
/* Helpers                                                                   */
/* ========================================================================= */

static void cleanup_channel(int i) {
    if (audio_state.channels[i].playing) {
        ma_sound_uninit(&audio_state.channels[i].sound);
        audio_state.channels[i].playing = 0;
    }
}

/* ========================================================================= */
/* Sound Effects                                                             */
/* ========================================================================= */

/*
** engine.Audio.play(path [, volume, loop, pitch]) -> channelId
*/
static int audio_play(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    float vol = (float)luaL_optnumber(L, 2, 1.0);
    int loop = lua_toboolean(L, 3);
    float pitch = (float)luaL_optnumber(L, 4, 1.0);
    int i;

    if (!audio_state.initialized) {
        return luaL_error(L, "Audio engine not initialized");
    }

    /* Find a free channel or clean up finished ones */
    int channel = -1;
    for (i = 0; i < MAX_AUDIO_CHANNELS; i++) {
        if (audio_state.channels[i].playing) {
            if (ma_sound_at_end(&audio_state.channels[i].sound)) {
                cleanup_channel(i);
            }
        }
        if (!audio_state.channels[i].playing && channel == -1) {
            channel = i;
        }
    }

    if (channel == -1) {
        return luaL_error(L, "no free audio channels available");
    }

    ma_result result = ma_sound_init_from_file(&audio_state.engine, path, 0, NULL, NULL, &audio_state.channels[channel].sound);
    if (result != MA_SUCCESS) {
        return luaL_error(L, "failed to load sound file: %s", path);
    }

    ma_sound_set_volume(&audio_state.channels[channel].sound, vol);
    ma_sound_set_looping(&audio_state.channels[channel].sound, loop);
    ma_sound_set_pitch(&audio_state.channels[channel].sound, pitch);
    ma_sound_start(&audio_state.channels[channel].sound);
    
    audio_state.channels[channel].playing = 1;

    lua_pushinteger(L, channel + 1);  /* 1-indexed channel ID */
    return 1;
}

/*
** engine.Audio.stop([channelId])
*/
static int audio_stop(lua_State *L) {
    if (lua_isnoneornil(L, 1)) {
        int i;
        for (i = 0; i < MAX_AUDIO_CHANNELS; i++) {
            cleanup_channel(i);
        }
    } else {
        int ch = (int)luaL_checkinteger(L, 1) - 1;
        if (ch >= 0 && ch < MAX_AUDIO_CHANNELS) {
            cleanup_channel(ch);
        }
    }
    return 0;
}

/*
** engine.Audio.pause(channelId)
*/
static int audio_pause(lua_State *L) {
    int ch = (int)luaL_checkinteger(L, 1) - 1;
    if (ch >= 0 && ch < MAX_AUDIO_CHANNELS && audio_state.channels[ch].playing) {
        ma_sound_stop(&audio_state.channels[ch].sound);
    }
    return 0;
}

/*
** engine.Audio.resume(channelId)
*/
static int audio_resume(lua_State *L) {
    int ch = (int)luaL_checkinteger(L, 1) - 1;
    if (ch >= 0 && ch < MAX_AUDIO_CHANNELS && audio_state.channels[ch].playing) {
        ma_sound_start(&audio_state.channels[ch].sound);
    }
    return 0;
}

/*
** engine.Audio.setVolume(volume)
*/
static int audio_set_volume(lua_State *L) {
    float vol = (float)luaL_checknumber(L, 1);
    ma_engine_set_volume(&audio_state.engine, vol);
    return 0;
}

/*
** engine.Audio.getVolume() -> number
*/
static int audio_get_volume(lua_State *L) {
    lua_pushnumber(L, ma_engine_get_volume(&audio_state.engine));
    return 1;
}

/*
** engine.Audio.setChannelVolume(channelId, volume)
*/
static int audio_set_channel_volume(lua_State *L) {
    int ch = (int)luaL_checkinteger(L, 1) - 1;
    float vol = (float)luaL_checknumber(L, 2);
    if (ch >= 0 && ch < MAX_AUDIO_CHANNELS && audio_state.channels[ch].playing) {
        ma_sound_set_volume(&audio_state.channels[ch].sound, vol);
    }
    return 0;
}

/*
** engine.Audio.isPlaying(channelId) -> boolean
*/
static int audio_is_playing(lua_State *L) {
    int ch = (int)luaL_checkinteger(L, 1) - 1;
    if (ch >= 0 && ch < MAX_AUDIO_CHANNELS && audio_state.channels[ch].playing) {
        lua_pushboolean(L, ma_sound_is_playing(&audio_state.channels[ch].sound));
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}

/* ========================================================================= */
/* Registration                                                              */
/* ========================================================================= */

static const luaL_Reg audio_funcs[] = {
    {"play",             audio_play},
    {"stop",             audio_stop},
    {"pause",            audio_pause},
    {"resume",           audio_resume},
    {"setVolume",        audio_set_volume},
    {"getVolume",        audio_get_volume},
    {"setChannelVolume", audio_set_channel_volume},
    {"isPlaying",        audio_is_playing},
    {NULL, NULL}
};

void engine_register_audio(lua_State *L) {
    if (!audio_state.initialized) {
        ma_result result = ma_engine_init(NULL, &audio_state.engine);
        if (result != MA_SUCCESS) {
            fprintf(stderr, "Failed to initialize audio engine.\n");
            /* We don't error out here to let the rest of the engine run */
            return;
        }
        audio_state.initialized = 1;
        memset(audio_state.channels, 0, sizeof(audio_state.channels));
    }

    luaL_newlib(L, audio_funcs);
    lua_setfield(L, -2, "Audio");
}
