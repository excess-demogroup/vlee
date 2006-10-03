// #include "file/file.h"
#include "stdafx.h"
#include "textureproxy.h"

using namespace engine;

TextureProxy::TextureProxy(IDirect3DDevice9 *device) : device(device)
{
}

core::Texture* TextureProxy::load(std::string filename)
{
	throw core::FatalException("homo!");
}

std::map<core::Texture*, int> ResourceProxy<core::Texture>::ref_count_static;
std::map<std::string, core::Texture*> ResourceProxy<core::Texture>::filename_map_static;
