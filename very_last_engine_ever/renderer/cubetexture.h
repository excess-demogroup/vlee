#pragma once

#include "texture.h"
#include "core/comref.h"

namespace renderer {
	class CubeTexture : public ComRef<IDirect3DCubeTexture9> {
	public:
		CubeTexture(IDirect3DCubeTexture9 *cubeTexture = NULL)
			: ComRef<IDirect3DCubeTexture9>(cubeTexture)
		{
		}
		
		virtual ~CubeTexture()
		{
		}
	};
}
