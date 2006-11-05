#pragma once

#include "../core/log.h"
#include "../core/fatalexception.h"
#include "../core/err.h"

#include "surface.h"

namespace renderer
{

	class Texture : public CComPtr<IDirect3DTexture9>
	{
	public:
		Texture() : CComPtr<IDirect3DTexture9>() {}

		Texture(IDirect3DDevice9 *device, UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0) : CComPtr<IDirect3DTexture9>()
		{
			assert(NULL != device);
			core::log::printf("creating texture... ");

			IDirect3DTexture9 *texture;
			HRESULT res = device->CreateTexture(width, height, levels, usage, format, pool, &texture, handle);
			if (FAILED(res))
			{
				std::string base_message;
				if (usage & D3DUSAGE_RENDERTARGET) base_message = std::string("failed to create render target\n");
				else  base_message = std::string("failed to create texture\n");
				throw core::FatalException(base_message + std::string(DXGetErrorString9(res)) + std::string(" : ") + std::string(DXGetErrorDescription9(res)));
			}
			assert(NULL != texture);
			core::log::printf("done.\n");

			Attach(texture); // don't addref
		}

		const Surface get_surface(int level = 0) const
		{
			Surface surf;
			core::d3d_err(p->GetSurfaceLevel(0, &surf));
			return surf;
		}
	};

}
