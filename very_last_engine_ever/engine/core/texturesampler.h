#pragma once

/*
	a class that wrap the state of hardware texture-samplers.
	to be used in both fixed function and pixelshaders (?)
*/

class TextureSampler {
public:
	TextureSampler() : texture(0) {
		addressu = D3DTADDRESS_WRAP;
		addressv = D3DTADDRESS_WRAP;
		addressw = D3DTADDRESS_WRAP;
		magfilter = D3DTEXF_POINT;
		minfilter = D3DTEXF_POINT;
		mipfilter = D3DTEXF_NONE;
		mipmaplodbias = 0.f;
		maxmiplevel = 0;
		maxanistropy = 1;
	}

	void set(IDirect3DDevice9 *device, unsigned index) {
		assert(0 != device);

		device->SetTexture(index, (IDirect3DTexture9*)texture);
		device->SetSamplerState(index, D3DSAMP_ADDRESSU, addressu);
		device->SetSamplerState(index, D3DSAMP_ADDRESSV, addressv);
		device->SetSamplerState(index, D3DSAMP_ADDRESSW, addressw);
//		unimplemented: D3DSAMP_BORDERCOLOR
		device->SetSamplerState(index, D3DSAMP_MAGFILTER, magfilter);
		device->SetSamplerState(index, D3DSAMP_MAGFILTER, minfilter);
		device->SetSamplerState(index, D3DSAMP_MIPFILTER, mipfilter);
		device->SetSamplerState(index, D3DSAMP_MIPMAPLODBIAS, *(unsigned*)(&mipmaplodbias));
		device->SetSamplerState(index, D3DSAMP_MAXMIPLEVEL, maxmiplevel);
		device->SetSamplerState(index, D3DSAMP_MAXANISOTROPY, maxanistropy);
//		unimplemented: D3DSAMP_SRGBTEXTURE
//		unimplemented: D3DSAMP_ELEMENTINDEX
//		unimplemented: D3DSAMP_DMAPOFFSET
	}

	void set_texture(Texture *texture) {
		this->texture = texture;
	}

protected:
	D3DTEXTUREADDRESS addressu, addressv, addressw;
	D3DTEXTUREFILTERTYPE magfilter, minfilter, mipfilter;
	FLOAT mipmaplodbias;
	DWORD maxmiplevel;
	DWORD maxanistropy;

	Texture *texture;
};

