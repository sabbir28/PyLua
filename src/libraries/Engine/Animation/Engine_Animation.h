#ifndef ENGINE_ANIMATION_H
#define ENGINE_ANIMATION_H

#include "../../../../include/lua.h"
#include "../Assets/Engine_Assets.h"

typedef struct {
    Texture *texture;
    int frameWidth;
    int frameHeight;
    int cols;
    int rows;
} SpriteSheet;

typedef struct {
    int frameIndex;
    float duration;
} Keyframe;

typedef struct {
    char name[32];
    Keyframe *keyframes;
    int keyframeCount;
} AnimationSequence;

typedef struct {
    SpriteSheet *sheet;
    AnimationSequence *sequences;
    int sequenceCount;
    int currentSequence;
    int sheet_ref;
} Animator;

/* Legacy support or single-sequence shorthand */
typedef struct {
    SpriteSheet *sheet;
    Keyframe *keyframes;
    int keyframeCount;
    int loop;
    int sheet_ref;
} Animation;

void engine_register_animation(lua_State *L);

#endif /* ENGINE_ANIMATION_H */
