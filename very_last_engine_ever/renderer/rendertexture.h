#pragma once

#include "texture.h"

namespace renderer
{

	class RenderTexture : public Texture
	{
	public:
		RenderTexture() {}
		RenderTexture(IDirect3DDevice9 *device, UINT width, UINT height, UINT levels = 1, D3DFORMAT format = D3DFMT_A8R8G8B8,  D3DMULTISAMPLE_TYPE multisample = D3DMULTISAMPLE_NONE)
			: Texture(device, width, height, levels, D3DUSAGE_RENDERTARGET, format), device(device), shadow_surf(NULL), texture_surf(NULL)
		{
			if (D3DMULTISAMPLE_NONE != multisample)
			{
				core::d3d_err(
					device->CreateRenderTarget(
						width, height,
						format, multisample,
						0, false,
						&shadow_surf, NULL
					)
				);
			}
			
			texture_surf = this->getSurface(0);
		}
		
		operator IDirect3DSurface9*()
		{
			if (NULL != shadow_surf) return shadow_surf;
			return texture_surf;
		}
		
		operator Surface()
		{
			if (NULL != shadow_surf) return Surface(shadow_surf);
			return Surface(texture_surf);
		}
		
		void resolve()
		{
			if (NULL != shadow_surf) device->StretchRect(shadow_surf, NULL, texture_surf, NULL, D3DTEXF_NONE);
		}

	private:
		IDirect3DDevice9 *device;
		IDirect3DSurface9 *shadow_surf;
		IDirect3DSurface9 *texture_surf;
	};

};
