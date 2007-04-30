#pragma once

#include "../renderer/texture.h"
#include "../renderer/device.h"

namespace engine
{
	class Anim
	{
	public:
		renderer::Texture &getFrame(float pos)
		{
			assert(0 != textures.size());
			int idx = int(pos * textures.size());
			idx %= textures.size();
			return textures[idx];
		}

		renderer::Texture &getFramePingPong(float pos)
		{
			assert(0 != textures.size());
			int idx = int(pos * textures.size());
			idx %= (textures.size() * 2) - 2;
			if (idx >= int(textures.size())) idx = textures.size() - 1 - (idx - textures.size());
			assert(idx >= 0);
			assert(idx < int(textures.size()));
			return textures[idx];
		}

		std::vector<renderer::Texture> textures;
	};

	Anim load_anim(renderer::Device &device, std::string folder);
}
