#include "Include.h"
#ifndef PROJECTILE
#define PROJECTILE
struct Projectile {
    size_t x, y;
    int direction; // Towards aliens (+), Towards player (-)
};
#endif