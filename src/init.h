#pragma once

namespace init
{
	D3DFORMAT get_best_depth_stencil_format(IDirect3D9 *direct3d, UINT adapter, D3DFORMAT format);

	IDirect3DDevice9 *initD3D(IDirect3D9 *direct3d, HWND win, D3DDISPLAYMODE mode, D3DMULTISAMPLE_TYPE multisample, unsigned adapter, bool vsync);
	IDirect3DSurface9 *getBackbuffer();
	void updateD3D();
}
