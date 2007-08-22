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
namespace renderer
{
	class Surface : public CComPtr<IDirect3DSurface9>
	{
	public:
		Surface(IDirect3DSurface9 *surface = 0) : CComPtr<IDirect3DSurface9>(surface)
		{
			
		}

		const D3DSURFACE_DESC get_desc() const
		{
			D3DSURFACE_DESC desc;
			p->GetDesc(&desc);
			return desc;
		}
	};

}
