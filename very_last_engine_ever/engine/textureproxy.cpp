// #include "file/file.h"
#include "stdafx.h"
#include "engine.h"

using namespace engine;
using namespace engine::core;

TextureProxy::TextureProxy(IDirect3DDevice9 *device) : device(device)
{}

Texture* TextureProxy::load(std::string filename)
{
//	file *fp = file_open(filename.c_str());
//	if (!fp) throw FatalException("file not found: ") + filename;
//	Texture *temp = new Texture;
//	if (FAILED(D3DXCreateTextureFromFileInMemory(device, fp->data, fp->size, &temp->texture))) throw std::exception("failed to load ")+filename;
//	file_close(fp);

	IDirect3DTexture9 *tex;
	if (FAILED(D3DXCreateTextureFromFile(device, filename.c_str(), &tex))) throw FatalException("D3DXCreateTextureFromFile() failed");
//	if (FAILED(D3DXCreateTextureFromFile(device, filename.c_str(), &tex))) throw std::exception((std::string("failed to load ") + filename).c_str());
	Texture *tex2 = new Texture;
	tex2->Attach(tex);
	return tex2;
}

std::map<Texture*, int> ResourceProxy<Texture>::ref_count_static;
std::map<std::string, Texture*> ResourceProxy<Texture>::filename_map_static;
