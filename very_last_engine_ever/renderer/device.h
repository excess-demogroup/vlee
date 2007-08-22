#pragma once

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

		/* vertex/index buffers */
		VertexBuffer createVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0);
		IndexBuffer createIndexBuffer(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0);
		VertexDeclaration createVertexDeclaration(CONST D3DVERTEXELEMENT9* vertex_elements);

		/* textures / surfaces */
		Texture createTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0);
		Surface createDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard );
	};
};
