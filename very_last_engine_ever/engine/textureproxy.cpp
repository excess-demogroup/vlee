// #include "file/file.h"
#include "stdafx.h"
#include "textureproxy.h"

using namespace engine;
using renderer::Texture;

TextureProxy::TextureProxy(IDirect3DDevice9 *device) : device(device)
{
}

renderer::Texture* TextureProxy::load(std::string filename)
{
	throw core::FatalException("homo!");
}

std::map<Texture*, int> ResourceProxy<Texture>::ref_count_static;
std::map<std::string, Texture*> ResourceProxy<Texture>::filename_map_static;
