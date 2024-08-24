#include "Include.h"
#ifndef BUFFER
#define BUFFER
struct Buffer {
    size_t width, height;
    uint32_t* data;
};
#endif