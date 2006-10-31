#pragma once

#include "texture.h"

namespace renderer
{

	class RenderTexture : public Texture
	{
	public:
		RenderTexture() {}
		RenderTexture(IDirect3DDevice9 *device, UINT width, UINT height, UINT levels = 1, D3DFORMAT format = D3DFMT_A8R8G8B8)
			: Texture(device, width, height, levels, D3DUSAGE_RENDERTARGET, format)
		{
		}
	};

};
