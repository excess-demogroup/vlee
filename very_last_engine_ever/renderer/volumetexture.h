#pragma once

#include "texture.h"

namespace renderer
{
	
	class VolumeTexture : public CComPtr<IDirect3DVolumeTexture9>
	{
	public:
		VolumeTexture(IDirect3DVolumeTexture9 *volumeTexture = NULL)
			: CComPtr<IDirect3DVolumeTexture9>(volumeTexture)
		{
		}
		
		virtual ~VolumeTexture()
		{
		}
	};
}
