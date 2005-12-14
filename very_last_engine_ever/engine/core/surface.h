#pragma once

/*
LOOK INTO:

HRESULT IDirect3DDevice9::CreateDepthStencilSurface(
	UINT Width,
	UINT Height,
	D3DFORMAT Format,
	D3DMULTISAMPLE_TYPE MultiSample,
	DWORD MultisampleQuality,
	BOOL Discard,
	IDirect3DSurface9** ppSurface,
	HANDLE* pSharedHandle
);

HRESULT IDirect3DDevice9::CreateOffscreenPlainSurface(
	UINT Width,
	UINT Height,
	D3DFORMAT Format,
	DWORD Pool,
	IDirect3DSurface9** ppSurface,
	HANDLE* pSharedHandle
);

HRESULT IDirect3DDevice9::CreateRenderTarget(
	UINT Width,
	UINT Height,
	D3DFORMAT Format,
	D3DMULTISAMPLE_TYPE MultiSample,
	DWORD MultisampleQuality,
	BOOL Lockable,
	IDirect3DSurface9** ppSurface,
	HANDLE* pSharedHandle
);
*/

class Surface {
public:
	Surface(IDirect3DSurface9 *surface = 0) : surface(surface) {}

	~Surface() {
		if (surface) surface->Release();
	}

	void set_render_target(IDirect3DDevice9 *device, unsigned index = 0) {
		assert(0 != device);
		assert(0 != surface);
		device->SetRenderTarget(index, surface);
	}

	static Surface get_render_target(IDirect3DDevice9 *device, unsigned index = 0) {
		assert(0 != device);
		IDirect3DSurface9 *surface;
		device->GetRenderTarget(index, &surface);
		return Surface(surface);
	}

	D3DSURFACE_DESC get_desc() const {
		D3DSURFACE_DESC desc;
		surface->GetDesc(&desc);
		return desc;
	}

	IDirect3DSurface9 *get_surface() const { return surface; }

protected:
	IDirect3DSurface9 *surface;
};
