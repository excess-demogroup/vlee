#pragma once

#include "surface.h"
#include "texture.h"
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
			core::d3d_err(p->SetRenderTarget(index, surface.p));
		}

		Surface getRenderTarget(unsigned index = 0)
		{
			assert(NULL != p);
			IDirect3DSurface9 *surface;
			core::d3d_err(p->GetRenderTarget(index, &surface));

/*			return Surface(surface); */

			Surface surface_wrapper;
			surface_wrapper.Attach(surface);
			return surface_wrapper;
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
			core::d3d_err(
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
