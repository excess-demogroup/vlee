#pragma once

#include "cubetexture.h"

namespace renderer {
	class RenderCubeTexture : public CubeTexture {
	public:
		RenderCubeTexture() {}
		RenderCubeTexture(Device &device, UINT size,
		    UINT levels = 1, D3DFORMAT format = D3DFMT_A8R8G8B8,
		    DWORD usage = 0)
			: CubeTexture(device, size, levels,
			    D3DUSAGE_RENDERTARGET | usage, format)
		{
		}

		virtual ~RenderCubeTexture()
		{
		}

		Surface getRenderTarget(D3DCUBEMAP_FACES face)
		{
			return getSurface(face);
		}

	private:
		bool multisampled;
		Surface shadow_surf;
		Surface texture_surf;
	};

};
