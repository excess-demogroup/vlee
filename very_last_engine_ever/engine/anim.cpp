#include "stdafx.h"
#include "anim.h"
#include "../core/fatalexception.h"

engine::Anim engine::loadAnim(renderer::Device &device, std::string folder)
{
	engine::Anim anim;
	for (int i = 0; true; ++i)
	{
		char temp[256];
		sprintf(temp, "%s/%04d.png", folder.c_str(), i);
		renderer::Texture tex;
		if (FAILED(D3DXCreateTextureFromFile(device, temp, &tex))) break;
		anim.addTexture(tex);
	}
	
	if (0 == anim.getTextureCount())
	{
		throw core::FatalException("no frames in video");
	}

	return anim;
}
