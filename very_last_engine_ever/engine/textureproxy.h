#pragma once

#include "../renderer/texture.h"
#include "../renderer/cubetexture.h"
#include "../renderer/volumetexture.h"

#include "../renderer/device.h"
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

	renderer::Texture       loadTexture(renderer::Device &device, ::std::string filename);
	renderer::CubeTexture   loadCubeTexture(renderer::Device &device, ::std::string filename);
	renderer::VolumeTexture loadVolumeTexture(renderer::Device &device, ::std::string filename);
}
