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
			return getTexture(size_t(idx));
		}
		
		renderer::Texture &getFramePingPong(float pos)
		{
			assert(0 != textures.size());
			int idx = int(pos * textures.size());
			idx %= (textures.size() * 2) - 1;
			if (idx >= int(textures.size())) idx = int(textures.size()) - 1 - (idx - int(textures.size()));
			return getTexture(size_t(idx));
		}
		
		renderer::Texture &getTexture(size_t idx)
		{
			assert(idx < textures.size());
			return textures[idx];
		}
		
		void addTexture(renderer::Texture &texture)
		{
			textures.push_back(texture);
		}
		
		size_t getTextureCount() { return textures.size(); }
		
	private:
		std::vector<renderer::Texture> textures;
	};

	Anim loadAnim(renderer::Device &device, std::string folder);
}
