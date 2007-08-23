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
	class VertexBuffer;
	class IndexBuffer;
	class VertexDeclaration;

	class Texture;
	class Surface;

	class Device : public CComPtr<IDirect3DDevice9>
	{
	public:
		
		/* render targets */
		void setRenderTarget(Surface &surface, unsigned index = 0);
		Surface getRenderTarget(unsigned index = 0);

		void setDepthStencilSurface(Surface &surface);
		Surface getDepthStencilSurface();

		/* vertex/index buffers */
		VertexBuffer createVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = NULL);
		IndexBuffer createIndexBuffer(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = NULL);
		VertexDeclaration createVertexDeclaration(CONST D3DVERTEXELEMENT9* vertex_elements);

		/* textures / surfaces */
		Texture createTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = NULL);
		Surface createDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard );
		Surface createRenderTarget(
			UINT Width,
			UINT Height,
			D3DFORMAT Format = D3DFMT_A8R8G8B8,
			D3DMULTISAMPLE_TYPE MultiSample = D3DMULTISAMPLE_NONE,
			DWORD MultisampleQuality = 0,
			BOOL Lockable = FALSE,
			HANDLE* pSharedHandle = NULL
		);

	};
};
