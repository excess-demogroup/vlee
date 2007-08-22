#pragma once

#include "surface.h"
#include "texture.h"
#include "vertexdeclaration.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "../core/err.h"

namespace renderer
{
	class Device : public CComPtr<IDirect3DDevice9>
	{
	public:

		void setRenderTarget(Surface &surface, unsigned index = 0)
		{
			assert(NULL != p);
			assert(NULL != surface);
			core::d3dErr(p->SetRenderTarget(index, surface.p));
		}

		Surface getRenderTarget(unsigned index = 0)
		{
			assert(NULL != p);
			IDirect3DSurface9 *surface;
			core::d3dErr(p->GetRenderTarget(index, &surface));

/*			return Surface(surface); */

			Surface surface_wrapper;
			surface_wrapper.Attach(surface);
			return surface_wrapper;
		}

		VertexBuffer createVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0)
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

		IndexBuffer createIndexBuffer(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0)
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

		VertexDeclaration createVertexDeclaration(CONST D3DVERTEXELEMENT9* vertex_elements)
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

		Texture createTexture(UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0)
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
				throw core::FatalException(base_message + std::string(DXGetErrorString9(res)) + std::string(" : ") + std::string(DXGetErrorDescription9(res)));
			}
			assert(NULL != texture);
			
			core::log::printf("done.\n");
			
			return Texture(texture);
		}

		Surface createDepthStencilSurface(
				UINT Width,
				UINT Height,
				D3DFORMAT Format,
				D3DMULTISAMPLE_TYPE MultiSample,
				DWORD MultisampleQuality,
				BOOL Discard
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
					NULL)
			);

			Surface surf;
			surf.Attach(d3d_surf);
			return surf;
		}

	};
};
