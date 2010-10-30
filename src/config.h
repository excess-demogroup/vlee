#pragma once

// #define VJSYS

#define DEVTYPE D3DDEVTYPE_HAL

#define DEFAULT_FORMAT D3DFMT_X8R8G8B8
#define DEFAULT_VSYNC TRUE

#define NEED_STENCIL TRUE
#define MIN_VS_VERSION D3DVS_VERSION(3, 0)
// #define MIN_VS_VERSION D3DVS_VERSION(1, 1)

#if defined(NDEBUG) && !defined(SYNC) && !defined(DUMP_VIDEO)
#define WINDOWED 0
#else
#define WINDOWED 1
#endif

#define DEMO_ASPECT (16.0 / 9)
#define BPM 127.05

#ifdef VJSYS
#define DEFAULT_SOUNDCARD 0
#else
#define DEFAULT_SOUNDCARD 1
#endif
