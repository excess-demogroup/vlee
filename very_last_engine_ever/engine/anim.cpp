#include "stdafx.h"
#include "anim.h"
#include "../core/fatalexception.h"

engine::Anim engine::load_anim(renderer::Device &device, std::string folder)
{
	engine::Anim anim;
	for (int i = 0; true; ++i)
	{
		char temp[256];
		sprintf(temp, "%s/%04d.jpg", folder.c_str(), i);
		renderer::Texture tex;
		if (FAILED(D3DXCreateTextureFromFile(device, temp, &tex))) break;
		anim.textures.push_back(tex);
	}

	if (0 == anim.textures.size())
	{
		throw core::FatalException("no frames in video");
	}

	return anim;
}
