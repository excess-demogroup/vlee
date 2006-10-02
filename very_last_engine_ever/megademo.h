#ifndef MEGADEMO_H
#define MEGADEMO_H

#define BPM 135
#define END_TIME (60 * 3 + 30) /* 3:30 */

namespace engine
{
	Texture load_texture(engine::core::Device &device, std::string filename)
	{
		Texture tex;

		HRESULT hr = D3DXCreateTextureFromFile(device, filename.c_str(), &tex);
		if (FAILED(hr)) throw core::FatalException(std::string("failed to load mesh \"") + filename + std::string("\"\n\n") + core::d3d_get_error(hr));

		return tex;
	}
}

void set_ramp(float alphas[], float base)
{
	for (unsigned i = 0; i < 8; i++)
	{
		alphas[i] = base;
		base *= 0.5f;
	}
}

void blit(IDirect3DDevice9 *device, IDirect3DTexture9 *tex, Effect &eff, Mesh &polygon)
{
	assert(polygon);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	eff->SetTexture("tex", tex);
	eff->CommitChanges();
	eff.draw(polygon);
}

class MegaDemo : public engine::Demo
{
private:

	Surface backbuffer;
	float aspect;

	Texture rtex;
	CComPtr<IDirect3DSurface9> rtex_surf;

	Texture bloomtex[2];
	CComPtr<IDirect3DSurface9> bloomtex_surf[2];

	Mesh polygon;
	Mesh mesh2;

	Effect eff;
	Effect blur_fx;
	Effect tex_fx;

	Texture tex;

	engine::TextureProxy texloader;

/*	engine::bass::Stream music;
	engine::core::Texture tex;
	engine::scenegraph::Scene scene; */

	Sync &sync;
	SyncTrack &fade, &flash, &part;
	SyncTrack &xrot, &yrot, &zrot;

public:
	MegaDemo(engine::core::Device &device, float aspect, Sync &sync) :
		Demo(device),
		aspect(aspect),
		sync(sync),
		fade( sync.getTrack("fade",     "global", 5, true)),
		flash(sync.getTrack("flash",    "global", 5, true)),
		part( sync.getTrack("part",     "global", 5, true)),
		xrot( sync.getTrack("x",        "rotation", 5, true)),
		yrot( sync.getTrack("y",        "rotation", 5, true)),
		zrot( sync.getTrack("z",        "rotation", 5, true)),
		texloader(device),
		backbuffer(Surface::get_render_target(device))
	{

/*		tex   = texloader.get("test.jpg");
		scene = sceneloader.get("test.scene");
		music = musicloader.get(); */

		rtex = Texture(device, backbuffer.get_desc().Width, backbuffer.get_desc().Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, 0);
		rtex_surf = CComPtr<IDirect3DSurface9>(rtex.get_surface(0));

		bloomtex[0] = Texture(device, backbuffer.get_desc().Width, backbuffer.get_desc().Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, 0);
		bloomtex[1] = Texture(device, backbuffer.get_desc().Width, backbuffer.get_desc().Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, 0);

		bloomtex_surf[0] = bloomtex[0].get_surface(0);
		bloomtex_surf[1] = bloomtex[1].get_surface(0);

		d3d_err(D3DXCreatePolygon(device, 3.f, 4, &polygon, 0));

		mesh2 = engine::load_mesh(device, "data/test.x");

		eff     = engine::load_effect(device, "data/test.fx");
		blur_fx = engine::load_effect(device, "data/blur.fx");
		tex_fx  = engine::load_effect(device, "data/tex.fx");

		tex     = engine::load_texture(device, "data/logo.png");

		CComPtr<IDirect3DCubeTexture9> env;
		d3d_err(D3DXCreateCubeTextureFromFile(device, "data/stpeters_cross2.dds", &env));
		eff->SetTexture("env", env);
	}

	void start()
	{
/*		music.play(); */
	}

	void draw(float time)
	{
		double beat = time * (double(BPM) / 60);

#ifdef VJSYS
		// is there any way of recieving _all_ fft-data without recording?
		while (BASS_ChannelGetData(stream, 0, BASS_DATA_AVAILABLE) >= 2048 * 2)
		{
			BASS_ChannelGetData(stream, spectrum, BASS_DATA_FFT512);
		}
#endif
		if (BASS_ChannelGetData(stream, 0, BASS_DATA_AVAILABLE) >= 2048)
		{
			BASS_ChannelGetData(stream, spectrum, BASS_DATA_FFT512);
		}

		device->SetRenderTarget(0, backbuffer.get_surface());
		D3DVIEWPORT9 viewport;
		device->GetViewport(&viewport);
		const float correction = (aspect * (9 / 16.f));
		int old_height = viewport.Height;
		int new_height = int(viewport.Height * correction);
		viewport.Y      = (old_height - new_height) / 2;
		viewport.Height = new_height;

		device->BeginScene();
		device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DXCOLOR(0.f, 0.f, 0.f, 0.f), 1.f, 0);
		device->SetViewport(&viewport);

		// setup projection
		D3DXMATRIX proj;

		D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(80.f), 16.f / 9, 1.f, 100000.f);
		device->SetTransform(D3DTS_PROJECTION, &proj);

		float rot = time;

		// setup camera (animate parameters)
		D3DXVECTOR3 at(0, 0.f, 10.f);
		D3DXVECTOR3 up(0.f, 0.f, 1.f);
		D3DXVECTOR3 eye(
			sin(rot * 0.25f) * 10,
			cos(rot * 0.25f) * 10,
			50.f);

		// setup camera (view matrix)
		D3DXMATRIX view;
		D3DXMatrixLookAtLH(&view, &eye, &at, &up);
		device->SetTransform(D3DTS_VIEW, &view);

		// setup object matrix
		D3DXMATRIX world;
		D3DXMatrixIdentity(&world);
		device->SetTransform(D3DTS_WORLD, &world);

		eff.set_matrices(world, view, proj);
		eff->SetTexture("map", tex);
		eff->CommitChanges();

		eff.draw(mesh2);

		device->EndScene();
		HRESULT res = device->Present(0, 0, 0, 0);
		if (FAILED(res))
		{
			throw FatalException(std::string(DXGetErrorString9(res)) + std::string(" : ") + std::string(DXGetErrorDescription9(res)));
		}

		if (time > END_TIME) done = true;
	}
};

#endif // MEGADEMO_H
