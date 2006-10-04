#pragma once

#include "../renderer/texture.h"
#include "resourceproxy.h"

namespace engine
{
	class TextureProxy : public ResourceProxy<renderer::Texture>
	{
	public:
		TextureProxy(IDirect3DDevice9 *device);
	private:
		renderer::Texture* load(std::string filename);
		IDirect3DDevice9 *device;
	};
}