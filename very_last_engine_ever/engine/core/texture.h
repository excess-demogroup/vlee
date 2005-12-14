#pragma once

class Texture {
public:
	Texture(IDirect3DTexture9 *texture, bool addref) : texture(texture) {
		if (texture) texture->AddRef();
	}

	Texture(IDirect3DDevice9 *device, UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool = D3DPOOL_DEFAULT, HANDLE* handle = 0) : texture(0) {
		assert(0 != device);

		engine::core::log::printf("creating texture... ");

		HRESULT res = device->CreateTexture(width, height, levels, usage, format, pool, &texture, handle);
		if (FAILED(res)) {
			std::string base_message;
			if (usage & D3DUSAGE_RENDERTARGET) base_message = std::string("failed to create render target\n");
			else  base_message = std::string("failed to create texture\n");
			throw FatalException(base_message + std::string(DXGetErrorString9(res)) + std::string(" : ") + std::string(DXGetErrorDescription9(res)));
		}
		assert(0 != texture);
		engine::core::log::printf("done.\n");
	}

	~Texture() {
		if (texture) {
			texture->Release();
			texture = 0;
		}
	}

	IDirect3DTexture9 *get_texture() const { return texture; }

	IDirect3DSurface9 *get_surface(unsigned level) const {
		IDirect3DSurface9 *surf;
		if (FAILED(texture->GetSurfaceLevel(level, &surf))) throw FatalException("failed to get surface from texture");
		return surf;
	}

protected:
	IDirect3DTexture9 *texture;
};
