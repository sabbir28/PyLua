#include "Engine_Animation.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../../../../include/lua.h"
#include "../../../../include/lauxlib.h"
#include "../../../../include/lualib.h"

#define ENGINE_SPRITESHEET_MT "Engine.SpriteSheet"
#define ENGINE_ANIMATION_MT "Engine.Animation"
#define ENGINE_ANIMATOR_MT "Engine.Animator"

/* ========================================================================= */
/* SpriteSheet                                                               */
/* ========================================================================= */

static int spritesheet_new(lua_State *L) {
    Texture *tex = (Texture *)luaL_checkudata(L, 1, "Engine.Texture");
    int frameW = (int)luaL_checkinteger(L, 2);
    int frameH = (int)luaL_checkinteger(L, 3);

    /* ISSUE: Missing positive bounds check on frameW/frameH allowed division-by-zero crashes (SIGFPE).
       FIX: Added validation to ensure frame dimensions are strictly positive. */
    if (frameW <= 0 || frameH <= 0) {
        return luaL_error(L, "SpriteSheet frame width and height must be strictly positive (got %dx%d)", frameW, frameH);
    }

    SpriteSheet *ss = (SpriteSheet *)lua_newuserdatauv(L, sizeof(SpriteSheet), 1);
    ss->texture = tex;
    ss->frameWidth = frameW;
    ss->frameHeight = frameH;
    ss->cols = tex->width / frameW;
    ss->rows = tex->height / frameH;

    /* Associate the texture with the spritesheet to prevent GC */
    lua_pushvalue(L, 1);
    lua_setiuservalue(L, -2, 1);

    luaL_getmetatable(L, ENGINE_SPRITESHEET_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int spritesheet_get_info(lua_State *L) {
    SpriteSheet *ss = (SpriteSheet *)luaL_checkudata(L, 1, ENGINE_SPRITESHEET_MT);
    lua_pushinteger(L, ss->cols);
    lua_pushinteger(L, ss->rows);
    lua_pushinteger(L, ss->cols * ss->rows);
    return 3;
}

/* ========================================================================= */
/* Animation                                                                 */
/* ========================================================================= */

static int animation_new(lua_State *L) {
    SpriteSheet *ss = (SpriteSheet *)luaL_checkudata(L, 1, ENGINE_SPRITESHEET_MT);
    luaL_checktype(L, 2, LUA_TTABLE);
    int loop = lua_toboolean(L, 3);

    int keyframeCount = (int)luaL_len(L, 2);
    Animation *anim = (Animation *)lua_newuserdatauv(L, sizeof(Animation), 1);
    anim->sheet = ss;
    anim->keyframeCount = keyframeCount;
    anim->loop = loop;
    anim->keyframes = (Keyframe *)malloc(sizeof(Keyframe) * keyframeCount);

    for (int i = 1; i <= keyframeCount; i++) {
        lua_rawgeti(L, 2, i);
        if (lua_istable(L, -1)) {
            lua_getfield(L, -1, "frame");
            anim->keyframes[i - 1].frameIndex = (int)luaL_checkinteger(L, -1);
            lua_pop(L, 1);
            lua_getfield(L, -1, "duration");
            anim->keyframes[i - 1].duration = (float)luaL_optnumber(L, -1, 0.1);
            lua_pop(L, 1);
        } else {
            /* Support simple frame index list too */
            anim->keyframes[i - 1].frameIndex = (int)luaL_checkinteger(L, -1);
            anim->keyframes[i - 1].duration = 0.1f;
        }
        lua_pop(L, 1);
    }

    /* Store the spritesheet reference */
    lua_pushvalue(L, 1);
    lua_setiuservalue(L, -2, 1);

    luaL_getmetatable(L, ENGINE_ANIMATION_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int animation_gc(lua_State *L) {
    Animation *anim = (Animation *)luaL_checkudata(L, 1, ENGINE_ANIMATION_MT);
    if (anim->keyframes) {
        free(anim->keyframes);
        anim->keyframes = NULL;
    }
    return 0;
}

/* ========================================================================= */
/* Rendering                                                                 */
/* ========================================================================= */

static void draw_frame(SpriteSheet *ss, int frameIndex, float x, float y, float w, float h) {
    if (frameIndex < 0 || frameIndex >= ss->cols * ss->rows) return;

    int ix = frameIndex % ss->cols;
    int iy = frameIndex / ss->cols;

    float tw = (float)ss->texture->width;
    float th = (float)ss->texture->height;

    float u1 = (ix * ss->frameWidth) / tw;
    float v1 = (iy * ss->frameHeight) / th;
    float u2 = ((ix + 1) * ss->frameWidth) / tw;
    float v2 = ((iy + 1) * ss->frameHeight) / th;

    glBindTexture(GL_TEXTURE_2D, ss->texture->id);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
        glTexCoord2f(u1, v1); glVertex2f(x, y);
        glTexCoord2f(u2, v1); glVertex2f(x + w, y);
        glTexCoord2f(u2, v2); glVertex2f(x + w, y + h);
        glTexCoord2f(u1, v2); glVertex2f(x, y + h);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

static int animation_draw(lua_State *L) {
    Animation *anim = (Animation *)luaL_checkudata(L, 1, ENGINE_ANIMATION_MT);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float time = (float)luaL_checknumber(L, 4);
    float w = (float)luaL_optnumber(L, 5, anim->sheet->frameWidth);
    float h = (float)luaL_optnumber(L, 6, anim->sheet->frameHeight);

    if (anim->keyframeCount == 0) return 0;

    float totalDuration = 0;
    for (int i = 0; i < anim->keyframeCount; i++) {
        totalDuration += anim->keyframes[i].duration;
    }

    float t = time;
    if (anim->loop) {
        t = fmodf(time, totalDuration);
    } else if (t > totalDuration) {
        t = totalDuration - 0.0001f;
    }

    float currentSum = 0;
    int frameIndex = anim->keyframes[0].frameIndex;
    for (int i = 0; i < anim->keyframeCount; i++) {
        if (t < currentSum + anim->keyframes[i].duration) {
            frameIndex = anim->keyframes[i].frameIndex;
            break;
        }
        currentSum += anim->keyframes[i].duration;
    }

    draw_frame(anim->sheet, frameIndex, x, y, w, h);

    return 0;
}

/* ========================================================================= */
/* Registration                                                              */
/* ========================================================================= */

static int animator_play(lua_State *L) {
    Animator *anim = (Animator *)luaL_checkudata(L, 1, ENGINE_ANIMATOR_MT);
    const char *name = luaL_checkstring(L, 2);
    for (int i = 0; i < anim->sequenceCount; i++) {
        if (strcmp(anim->sequences[i].name, name) == 0) {
            anim->currentSequence = i;
            return 0;
        }
    }
    return luaL_error(L, "Animation sequence not found: %s", name);
}

static int animator_draw(lua_State *L) {
    Animator *anim = (Animator *)luaL_checkudata(L, 1, ENGINE_ANIMATOR_MT);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float time = (float)luaL_checknumber(L, 4);
    float w = (float)luaL_optnumber(L, 5, anim->sheet->frameWidth);
    float h = (float)luaL_optnumber(L, 6, anim->sheet->frameHeight);

    if (anim->sequenceCount == 0 || anim->currentSequence < 0) return 0;
    AnimationSequence *seq = &anim->sequences[anim->currentSequence];
    if (seq->keyframeCount == 0) return 0;

    float totalDuration = 0;
    for (int i = 0; i < seq->keyframeCount; i++) totalDuration += seq->keyframes[i].duration;

    float t = fmodf(time, totalDuration);
    float currentSum = 0;
    int frameIndex = seq->keyframes[0].frameIndex;
    for (int i = 0; i < seq->keyframeCount; i++) {
        if (t < currentSum + seq->keyframes[i].duration) {
            frameIndex = seq->keyframes[i].frameIndex;
            break;
        }
        currentSum += seq->keyframes[i].duration;
    }
    draw_frame(anim->sheet, frameIndex, x, y, w, h);
    return 0;
}

static int animator_gc(lua_State *L) {
    Animator *anim = (Animator *)luaL_checkudata(L, 1, ENGINE_ANIMATOR_MT);
    for (int i = 0; i < anim->sequenceCount; i++) {
        if (anim->sequences[i].keyframes) free(anim->sequences[i].keyframes);
    }
    if (anim->sequences) free(anim->sequences);
    return 0;
}

static int animator_load_file(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    FILE *f = fopen(path, "rb");
    if (!f) return luaL_error(L, "Could not open animator file: %s", path);

    /* ISSUE: Missing bounds validation on path_len, name_len, frame dimensions, and sequence/keyframe counts
       allowed buffer overflows in tex_path[256] and anim->sequences[i].name[32], as well as division-by-zero crashes.
       FIX: Added strict header verification, length limits, dimension checks, and allocation error handling. */
    char header[6];
    if (fread(header, 1, 6, f) != 6 || memcmp(header, "PANIM", 5) != 0 || header[5] < '\x04') {
        fclose(f);
        return luaL_error(L, "Invalid or incompatible animator file format (requires PANIM v4+)");
    }

    int path_len = 0;
    if (fread(&path_len, 4, 1, f) != 1 || path_len <= 0 || path_len >= 256) {
        fclose(f);
        return luaL_error(L, "Malformed texture path length in animator file: %d", path_len);
    }
    char tex_path[256];
    if ((int)fread(tex_path, 1, (size_t)path_len, f) != path_len) {
        fclose(f);
        return luaL_error(L, "Truncated texture path in animator file");
    }
    tex_path[path_len] = '\0';

    lua_getglobal(L, "engine");
    lua_getfield(L, -1, "Assets");
    lua_getfield(L, -1, "loadTexture");
    lua_pushstring(L, tex_path);
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        fclose(f);
        return lua_error(L);
    }
    Texture *tex = (Texture *)luaL_checkudata(L, -1, "Engine.Texture");

    int frameW = 0, frameH = 0;
    if (fread(&frameW, 4, 1, f) != 1 || fread(&frameH, 4, 1, f) != 1 || frameW <= 0 || frameH <= 0) {
        fclose(f);
        return luaL_error(L, "Invalid frame dimensions (%dx%d) in animator file", frameW, frameH);
    }

    SpriteSheet *ss = (SpriteSheet *)lua_newuserdatauv(L, sizeof(SpriteSheet), 1);
    ss->texture = tex;
    ss->frameWidth = frameW;
    ss->frameHeight = frameH;
    ss->cols = tex->width / frameW;
    ss->rows = tex->height / frameH;
    lua_pushvalue(L, -2);
    lua_setiuservalue(L, -2, 1);
    luaL_getmetatable(L, ENGINE_SPRITESHEET_MT);
    lua_setmetatable(L, -2);

    int seqCount = 0;
    if (fread(&seqCount, 4, 1, f) != 1 || seqCount <= 0 || seqCount > 128) {
        fclose(f);
        return luaL_error(L, "Invalid sequence count (%d) in animator file", seqCount);
    }

    Animator *anim = (Animator *)lua_newuserdatauv(L, sizeof(Animator), 1);
    anim->sheet = ss;
    anim->sequenceCount = seqCount;
    anim->currentSequence = 0;
    anim->sequences = (AnimationSequence *)calloc((size_t)seqCount, sizeof(AnimationSequence));
    if (!anim->sequences) {
        fclose(f);
        return luaL_error(L, "Out of memory allocating %d animation sequences", seqCount);
    }

    for (int i = 0; i < seqCount; i++) {
        int name_len = 0;
        if (fread(&name_len, 4, 1, f) != 1 || name_len < 0 || name_len >= 32) {
            fclose(f);
            return luaL_error(L, "Invalid sequence name length (%d) in animator file", name_len);
        }
        if (name_len > 0 && (int)fread(anim->sequences[i].name, 1, (size_t)name_len, f) != name_len) {
            fclose(f);
            return luaL_error(L, "Truncated sequence name in animator file");
        }
        anim->sequences[i].name[name_len] = '\0';

        int kfCount = 0;
        if (fread(&kfCount, 4, 1, f) != 1 || kfCount < 0 || kfCount > 1024) {
            fclose(f);
            return luaL_error(L, "Invalid keyframe count (%d) in animator sequence '%s'", kfCount, anim->sequences[i].name);
        }
        anim->sequences[i].keyframeCount = kfCount;
        if (kfCount > 0) {
            anim->sequences[i].keyframes = (Keyframe *)calloc((size_t)kfCount, sizeof(Keyframe));
            if (!anim->sequences[i].keyframes) {
                fclose(f);
                return luaL_error(L, "Out of memory allocating %d keyframes", kfCount);
            }
            for (int k = 0; k < kfCount; k++) {
                if (fread(&anim->sequences[i].keyframes[k].frameIndex, 4, 1, f) != 1 ||
                    fread(&anim->sequences[i].keyframes[k].duration, 4, 1, f) != 1) {
                    fclose(f);
                    return luaL_error(L, "Truncated keyframe data in animator sequence '%s'", anim->sequences[i].name);
                }
            }
        } else {
            anim->sequences[i].keyframes = NULL;
        }
    }
    fclose(f);
    lua_pushvalue(L, -2);
    lua_setiuservalue(L, -2, 1);
    luaL_getmetatable(L, ENGINE_ANIMATOR_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int animation_load_file(lua_State *L) {
    return animator_load_file(L);
}

static const luaL_Reg ss_methods[] = {
    {"getInfo", spritesheet_get_info},
    {NULL, NULL}
};

static const luaL_Reg anim_methods[] = {
    {"draw", animation_draw},
    {"__gc", animation_gc},
    {NULL, NULL}
};

static const luaL_Reg animator_methods[] = {
    {"play", animator_play},
    {"draw", animator_draw},
    {"__gc", animator_gc},
    {NULL, NULL}
};

static const luaL_Reg anim_funcs[] = {
    {"newSpriteSheet", spritesheet_new},
    {"new",            animation_new},
    {"load",           animation_load_file},
    {"loadAnimator",   animator_load_file},
    {NULL, NULL}
};

void engine_register_animation(lua_State *L) {
    /* SpriteSheet Metatable */
    luaL_newmetatable(L, ENGINE_SPRITESHEET_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, ss_methods, 0);
    lua_pop(L, 1);

    /* Animation Metatable */
    luaL_newmetatable(L, ENGINE_ANIMATION_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, anim_methods, 0);
    lua_pop(L, 1);

    /* Animator Metatable */
    luaL_newmetatable(L, ENGINE_ANIMATOR_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, animator_methods, 0);
    lua_pop(L, 1);

    luaL_newlib(L, anim_funcs);
    lua_setfield(L, -2, "Animation");
}
