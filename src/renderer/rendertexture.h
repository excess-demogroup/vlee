#pragma once

#include "texture.h"

namespace renderer {
	class RenderTexture : public Texture {
	public:
		RenderTexture() {}
		RenderTexture(Device &device, UINT width, UINT height,
		    UINT levels = 1, D3DFORMAT format = D3DFMT_A8R8G8B8,
		    D3DMULTISAMPLE_TYPE multisample = D3DMULTISAMPLE_NONE,
		    DWORD usage = D3DUSAGE_RENDERTARGET)
			: Texture(device, width, height, levels, usage, format)
		{
			multisampled = D3DMULTISAMPLE_NONE != multisample;

			if (multisampled) {
				shadow_surf = device.createRenderTarget(
					width, height,
					format, multisample,
					0, false, NULL
				);
			}
			texture_surf = this->getSurface(0);
		}

		virtual ~RenderTexture()
		{
		}

		Surface getRenderTarget()
		{
			if (multisampled)
				return shadow_surf;
			else
				return texture_surf;
		}

		void resolve(Device &device)
		{
			if (multisampled)
				device->StretchRect(shadow_surf, NULL, texture_surf, NULL, D3DTEXF_NONE);
		}

	private:
		bool multisampled;
		Surface shadow_surf;
		Surface texture_surf;
	};

};
