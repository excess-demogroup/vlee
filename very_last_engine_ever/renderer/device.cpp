#include "stdafx.h"
#include "device.h"

#include "surface.h"
#include "texture.h"
#include "vertexdeclaration.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"

#include "../core/err.h"

namespace renderer
{
	void Device::setRenderTarget(Surface &surface, unsigned index)
	{
		assert(NULL != p);
		assert(NULL != surface);
		core::d3dErr(p->SetRenderTarget(index, surface.p));
	}

	Surface Device::getRenderTarget(unsigned index)
	{
		assert(NULL != p);
		IDirect3DSurface9 *surface;
		core::d3dErr(p->GetRenderTarget(index, &surface));

		Surface surface_wrapper;
		surface_wrapper.Attach(surface);
		return surface_wrapper;
	}

	void Device::setDepthStencilSurface(Surface &surface)
	{
		assert(NULL != p);
		assert(NULL != surface);
		core::d3dErr(p->SetDepthStencilSurface(surface.p));
	}

	Surface Device::getDepthStencilSurface()
	{
		assert(NULL != p);
		IDirect3DSurface9 *surface;
		core::d3dErr(p->GetDepthStencilSurface(&surface));
		
		Surface surface_wrapper;
		surface_wrapper.Attach(surface);
		return surface_wrapper;
	}


	VertexBuffer Device::createVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool, HANDLE* handle)
	{
		core::log::printf("creating vertexbuffer... ");

		assert(NULL != p);
		IDirect3DVertexBuffer9 *vb;
		if (FAILED(p->CreateVertexBuffer(length, usage, fvf, pool, &vb, handle)))
		{
			throw core::FatalException("failed to create vertex buffer");
		}
		assert(NULL != vb);
		core::log::printf("done.\n");
		
		VertexBuffer vb_wrapper;
		vb_wrapper.Attach(vb); // don't addref
		return vb_wrapper;
	}

	IndexBuffer Device::createIndexBuffer(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool, HANDLE* handle)
	{
		core::log::printf("creating vertexbuffer... ");

		assert(NULL != p);
		IDirect3DIndexBuffer9 *ib;
		if (FAILED(p->CreateIndexBuffer(length, usage, format, pool, &ib, handle)))
		{
			throw core::FatalException("failed to create vertex buffer");
		}
		assert(NULL != ib);
		core::log::printf("done.\n");

		IndexBuffer ib_wrapper;
		ib_wrapper.Attach(ib); // don't addref
		return ib_wrapper;
	}

	VertexDeclaration Device::createVertexDeclaration(CONST D3DVERTEXELEMENT9* vertex_elements)
	{
		core::log::printf("creating vertexdeclaration... ");
		
		assert(0 != p);
		IDirect3DVertexDeclaration9 *decl;
		if (FAILED(p->CreateVertexDeclaration(vertex_elements, &decl)))
			throw core::FatalException("failed to create vertex declaration");
		core::log::printf("done.\n");
		
		VertexDeclaration decl_wrapper;
		decl_wrapper.Attach(decl); // don't addref
		return decl_wrapper;
	}

	Texture Device::createTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, HANDLE* handle)
	{
		assert(NULL != p);
		
		core::log::printf("creating texture... ");
		
		IDirect3DTexture9 *texture;
		HRESULT res = p->CreateTexture(width, height, levels, usage, format, pool, &texture, handle);
		if (FAILED(res))
		{
			std::string base_message;
			if (usage & D3DUSAGE_RENDERTARGET) base_message = std::string("failed to create render target\n");
			else  base_message = std::string("failed to create texture\n");
			throw core::FatalException(base_message + std::string(DXGetErrorString(res)) + std::string(" : ") + std::string(DXGetErrorDescription(res)));
		}
		assert(NULL != texture);
		
		core::log::printf("done.\n");
		
		return Texture(texture);
	}

	Surface Device::createRenderTarget(
		UINT Width,
		UINT Height,
		D3DFORMAT Format,
		D3DMULTISAMPLE_TYPE MultiSample,
		DWORD MultisampleQuality,
		BOOL Lockable,
		HANDLE* pSharedHandle
		)
	{
		IDirect3DSurface9 *surface;
		core::d3dErr(
			p->CreateRenderTarget(
				Width,
				Height,
				Format,
				MultiSample,
				MultisampleQuality,
				Lockable,
				&surface,
				pSharedHandle
			)
		);

		Surface surface_wrapper;
		surface_wrapper.Attach(surface);
		return surface_wrapper;
	}

	Surface Device::createDepthStencilSurface(
			UINT Width,
			UINT Height,
			D3DFORMAT Format,
			D3DMULTISAMPLE_TYPE MultiSample,
			DWORD MultisampleQuality,
			BOOL Discard,
			HANDLE* pSharedHandle
		)
	{
		IDirect3DSurface9 *d3d_surf;
		core::d3dErr(
			p->CreateDepthStencilSurface(
				Width,
				Height,
				Format,
				MultiSample,
				MultisampleQuality,
				Discard,
				&d3d_surf,
				pSharedHandle)
		);

		Surface surf;
		surf.Attach(d3d_surf);
		return surf;
	}
};
