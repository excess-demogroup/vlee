// #include "file/file.h"
#include "stdafx.h"
#include "textureproxy.h"

using namespace engine;
using renderer::Texture;

Texture engine::loadTexture(renderer::Device &device, ::std::string filename)
{
	IDirect3DTexture9 *texture;

	HRESULT hr = D3DXCreateTextureFromFileEx(
		device, filename.c_str(),
		D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, // width and height
		D3DX_DEFAULT, // miplevels
		0, D3DFMT_UNKNOWN, // usage and format
		D3DPOOL_MANAGED, // pool
		D3DX_DEFAULT, D3DX_DEFAULT, // filtering
		0, NULL, NULL,
		&texture
	);

	if (FAILED(hr)) throw core::FatalException(::std::string("failed to load mesh \"") + filename + ::std::string("\"\n\n") + core::d3dGetError(hr));

	Texture texture_wrapper;
	texture_wrapper.Attach(texture);
	return texture_wrapper;
}


TextureProxy::TextureProxy(IDirect3DDevice9 *device) : device(device)
{
}

Texture* TextureProxy::load(std::string filename)
{
	throw core::FatalException("homo!");
}

std::map<Texture*, int> ResourceProxy<Texture>::ref_count_static;
std::map<std::string, Texture*> ResourceProxy<Texture>::filename_map_static;
