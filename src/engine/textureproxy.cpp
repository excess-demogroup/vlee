// #include "file/file.h"
#include "stdafx.h"
#include "textureproxy.h"

using namespace engine;
using renderer::Texture;

Texture engine::loadTexture(renderer::Device &device, ::std::string filename)
{
	IDirect3DTexture9 *texture;

	HRESULT hr = D3DXCreateTextureFromFileEx(
		device,
		filename.c_str(),
		D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, // width and height
		D3DX_DEFAULT, // miplevels
		0, D3DFMT_UNKNOWN, // usage and format
		D3DPOOL_MANAGED, // pool
		D3DX_DEFAULT, D3DX_DEFAULT, // filtering
		0, NULL, NULL,
		&texture
	);

	if (FAILED(hr)) throw core::FatalException(::std::string("failed to load texture \"") + filename + ::std::string("\"\n\n") + core::d3dGetError(hr));

	Texture texture_wrapper;
	texture_wrapper.tex.attachRef(texture);
	return texture_wrapper;
}

using renderer::CubeTexture;
CubeTexture engine::loadCubeTexture(renderer::Device &device, ::std::string filename)
{
	IDirect3DCubeTexture9 *texture;
	
	HRESULT hr = D3DXCreateCubeTextureFromFileEx(
		device,
		filename.c_str(),
		D3DX_DEFAULT, // size
		D3DX_DEFAULT, // miplevels
		0, D3DFMT_UNKNOWN, // usage and format
		D3DPOOL_MANAGED, // pool
		D3DX_DEFAULT, D3DX_DEFAULT, // filtering
		0, NULL, NULL,
		&texture
	);
	
	if (FAILED(hr)) throw core::FatalException(::std::string("failed to load cube texture \"") + filename + ::std::string("\"\n\n") + core::d3dGetError(hr));
	
	CubeTexture texture_wrapper;
	texture_wrapper.tex.attachRef(texture);
	return texture_wrapper;
}

using renderer::VolumeTexture;
VolumeTexture engine::loadVolumeTexture(renderer::Device &device, ::std::string filename)
{
	IDirect3DVolumeTexture9 *texture;
	
	HRESULT hr = D3DXCreateVolumeTextureFromFileEx(
		device,
		filename.c_str(),
		D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, // width, height and depth
		D3DX_DEFAULT, // miplevels
		0, D3DFMT_UNKNOWN, // usage and format
		D3DPOOL_MANAGED, // pool
		D3DX_DEFAULT, D3DX_DEFAULT, // filtering
		0, NULL, NULL,
		&texture
	);
	
	if (FAILED(hr)) throw core::FatalException(::std::string("failed to load volume texture \"") + filename + ::std::string("\"\n\n") + core::d3dGetError(hr));
	
	VolumeTexture texture_wrapper;
	texture_wrapper.attachRef(texture);
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
