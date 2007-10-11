#pragma once

#include "texture.h"

namespace renderer
{
	
	class CubeTexture : public CComPtr<IDirect3DCubeTexture9>
	{
	public:
		CubeTexture(IDirect3DCubeTexture9 *cubeTexture = NULL)
			: CComPtr<IDirect3DCubeTexture9>(cubeTexture)
		{
		}
		
		virtual ~CubeTexture()
		{
		}
	};
}
