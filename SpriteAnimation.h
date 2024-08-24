#include "Include.h"
#include "Sprite.h"
#ifndef SPRITEANIMATION
#define SPRITEANIMATION
struct SpriteAnimation {
    bool loop;
    size_t numFrames;
    size_t frameDuration;
    size_t time;
    Sprite** frames;
};
#endif