#pragma once

#include "resource.h"

namespace config
{
	INT_PTR showDialog(HINSTANCE hInstance, IDirect3D9 *direct3d);

	extern IDirect3D9 *direct3d;
	extern UINT adapter;
	extern D3DDISPLAYMODE mode;
	extern D3DMULTISAMPLE_TYPE multisample;
	extern float aspect;
	extern bool vsync, fullscreen;
	extern unsigned soundcard;
};
