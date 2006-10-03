#pragma once

#include "core/texture.h"
#include "resourceproxy.h"

namespace engine
{
	class TextureProxy : public ResourceProxy<core::Texture>
	{
	public:
		TextureProxy(IDirect3DDevice9 *device);
	private:
		core::Texture* load(std::string filename);
		IDirect3DDevice9 *device;
	};
}