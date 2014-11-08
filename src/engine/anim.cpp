#include "stdafx.h"
#include "anim.h"
#include "../core/fatalexception.h"

engine::Anim engine::loadAnim(renderer::Device &device, std::string folder)
{
	engine::Anim anim;
	for (int i = 0; true; ++i)
	{
		char temp[256];
		sprintf(temp, "%s/%04d.dds", folder.c_str(), i);
		renderer::Texture tex;
		HRESULT hr = D3DXCreateTextureFromFileEx(device, temp, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_FROM_FILE, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tex.tex);
		if (hr == D3DXERR_INVALIDDATA) {
			sprintf(temp, "%s/%04d.png", folder.c_str(), i);
			hr = D3DXCreateTextureFromFileEx(device, temp, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_FROM_FILE, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tex.tex);
			if (hr == D3DXERR_INVALIDDATA) {
				sprintf(temp, "%s/%04d.jpg", folder.c_str(), i);
				hr = D3DXCreateTextureFromFileEx(device, temp, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_FROM_FILE, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &tex.tex);
				if (hr == D3DXERR_INVALIDDATA)
					break;
			}
		}
		core::d3dErr(hr);
		anim.addTexture(tex);
	}
	
	if (0 == anim.getTextureCount())
	{
		throw core::FatalException("no frames in video");
	}

	return anim;
}
