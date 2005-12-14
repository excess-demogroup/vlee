#pragma once

D3DFORMAT get_best_depth_stencil_format(IDirect3D9 *direct3d, UINT adapter, D3DFORMAT format);
IDirect3DDevice9 *init_d3d(IDirect3D9 *direct3d, HWND win, D3DDISPLAYMODE mode, D3DMULTISAMPLE_TYPE multisample, unsigned adapter, bool vsync);
void init_bass(HWND win, unsigned soundcard);

