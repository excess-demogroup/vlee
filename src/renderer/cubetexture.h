#pragma once

#include "texture.h"
#include "core/comref.h"

namespace renderer {
	class CubeTexture {
	public:
		CubeTexture(IDirect3DCubeTexture9 *cubeTexture = NULL)
			: tex(cubeTexture)
		{
		}

		CubeTexture(IDirect3DDevice9 *device, UINT size, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0)
		{
			assert(NULL != device);
			core::log::printf("creating texture... ");

			IDirect3DCubeTexture9 *texture;
			HRESULT res = device->CreateCubeTexture(size, levels, usage, format, pool, &texture, handle);
			if (FAILED(res)) {
				std::string base_message;
				if (usage & D3DUSAGE_RENDERTARGET)
					base_message = std::string("failed to create renderable cube texture\n");
				else
					base_message = std::string("failed to create cube texture\n");
				throw core::FatalException(base_message + std::string(DXGetErrorString(res)) + std::string(" : ") + std::string(DXGetErrorDescription(res)));
			}
			assert(NULL != texture);
			core::log::printf("done.\n");

			tex.attachRef(texture);
		}
		
		virtual ~CubeTexture()
		{
		}

		const Surface getSurface(D3DCUBEMAP_FACES face, int level = 0) const
		{
			IDirect3DSurface9 *surface;
			core::d3dErr(tex->GetCubeMapSurface(face, level, &surface));

			Surface surface_wrapper;
			surface_wrapper.attachRef(surface);
			return surface;
		}

		ComRef<IDirect3DCubeTexture9> tex;
	};
}
