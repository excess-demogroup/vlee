#pragma once

#include "../core/log.h"
#include "../core/fatalexception.h"
#include "../core/err.h"
#include "../core/comref.h"

#include "surface.h"

namespace renderer {
	class Texture {
	public:
		explicit Texture(IDirect3DTexture9 *texture = NULL) :
		    tex(texture)
		{
		}

		virtual ~Texture()
		{
		}

		Texture(IDirect3DDevice9 *device, UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0)
		{
			assert(NULL != device);
			core::log::printf("creating texture... ");

			IDirect3DTexture9 *texture;
			HRESULT res = device->CreateTexture(width, height, levels, usage, format, pool, &texture, handle);
			if (FAILED(res)) {
				std::string base_message;
				if (usage & D3DUSAGE_RENDERTARGET)
					base_message = std::string("failed to create render target\n");
				else
					base_message = std::string("failed to create texture\n");
				throw core::FatalException(base_message + std::string(DXGetErrorString(res)) + std::string(" : ") + std::string(DXGetErrorDescription(res)));
			}
			assert(NULL != texture);
			core::log::printf("done.\n");

			tex.attachRef(texture);
		}

		const Surface getSurface(int level = 0) const
		{
			IDirect3DSurface9 *surface;
			core::d3dErr(tex->GetSurfaceLevel(level, &surface));

			Surface surface_wrapper;
			surface_wrapper.attachRef(surface);
			return surface;
		}
		
		Surface getSurface(int level = 0)
		{
			IDirect3DSurface9 *surface;
			core::d3dErr(tex->GetSurfaceLevel(level, &surface));

			Surface surface_wrapper;
			surface_wrapper.attachRef(surface);
			return surface;
		}

		D3DSURFACE_DESC getLevelDesc(int level = 0) const
		{
			D3DSURFACE_DESC desc;
			tex->GetLevelDesc(level, &desc);
			return desc;
		}

		int getWidth() const
		{
			return getLevelDesc(0).Width;
		}

		int getHeight() const
		{
			return getLevelDesc(0).Height;
		}

		ComRef<IDirect3DTexture9> tex;
	};
}
