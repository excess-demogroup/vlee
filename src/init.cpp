#include "stdafx.h"
#include "config.h"
#include "init.h"

#include "core/fatalexception.h"
using core::FatalException;

IDirect3DDevice9 *init::initD3D(IDirect3D9 *direct3d, HWND win, D3DDISPLAYMODE mode, D3DMULTISAMPLE_TYPE multisample, unsigned adapter, bool vsync, bool fullscreen)
{
	D3DCAPS9 caps;
	direct3d->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps);
	DWORD tnl = D3DCREATE_HARDWARE_VERTEXPROCESSING;

	if (!((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) && (caps.VertexShaderVersion >= MIN_VS_VERSION))) {
		tnl = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		MessageBox(NULL, "your gpu is still from poland!", "performance warning", MB_OK | MB_ICONWARNING);
	}

	D3DPRESENT_PARAMETERS pp;
	ZeroMemory(&pp, sizeof(pp));
	pp.BackBufferWidth = mode.Width;
	pp.BackBufferHeight = mode.Height;
	pp.BackBufferFormat = mode.Format;
	pp.BackBufferCount = 1;
	pp.MultiSampleType = multisample;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.Windowed = !fullscreen;
/*	pp.EnableAutoDepthStencil = TRUE;
	pp.AutoDepthStencilFormat = init::get_best_depth_stencil_format(direct3d, adapter, mode.Format);
	pp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL; */

	pp.PresentationInterval = vsync ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
	pp.FullScreen_RefreshRateInHz = fullscreen ? mode.RefreshRate : 0;

	IDirect3DDevice9 *device;
	HRESULT result = direct3d->CreateDevice(adapter, D3DDEVTYPE_HAL, win, tnl, &pp, &device);

	if (FAILED(result)) {
		// in case the backbuffer-count was too high, try again (it should be turned down to max by the driver)
		result = direct3d->CreateDevice(adapter, D3DDEVTYPE_HAL, win, tnl, &pp, &device);
		if (FAILED(result))
			throw FatalException(std::string(DXGetErrorString(result)) + std::string(" : ") + std::string(DXGetErrorDescription(result)));
	}
	return device;
}
