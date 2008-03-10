// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// utilize win2k/xp functionality
#define _WIN32_WINNT 0x0500

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES
#include <windows.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <omp.h>

#ifndef NDEBUG
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

// atl-stuff, for the config-dialog.
#include <atlbase.h>
#include <atlapp.h>
extern CAppModule _Module;
#include <atlwin.h>

// ain't gonna be no bling without...
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr9.h>

// got a nick, know what's BASS!
#include <bass.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <float.h>
#include <memory.h>
#include <assert.h>
#include <math.h>

#pragma warning (disable : 4702) // warning C4702: unreachable code
#include <exception>
#include <stdexcept>
#include <vector>
#include <list>
#include <deque>
#include <map>
#include <string>
#include <iostream>
#include <strstream>
#pragma warning (default : 4702)

#include <assert.h>
// TODO: reference additional headers your program requires here
