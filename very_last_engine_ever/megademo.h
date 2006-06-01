#ifndef MEGADEMO_H
#define MEGADEMO_H

#define BPM 135
#define END_TIME (60 * 3 + 30) /* 3:30 */

Effect load_effect(IDirect3DDevice9 *device, const char * filename)
{
	Effect eff;

	ID3DXBuffer *err_buf = 0;
	HRESULT hr = D3DXCreateEffectFromFile(device, filename, NULL, NULL, 0, NULL, &eff, &err_buf);

	if (FAILED(hr))
	{
		if (err_buf == 0) d3d_err(hr);
		throw FatalException((const char*)err_buf->GetBufferPointer());
	}

	eff.update();
	return eff;
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

	Texture rtex;
	CComPtr<IDirect3DSurface9> rtex_surf;

	Texture bloomtex[2];
	CComPtr<IDirect3DSurface9> bloomtex_surf[2];

	Mesh polygon;
	Mesh mesh2;

	Effect eff;
	Effect blur_fx;
	Effect blit_fx;
	Effect brightpass_fx;
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
	MegaDemo(engine::core::Device &device, Sync &sync) :
		Demo(device),
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

		d3d_err(D3DXLoadMeshFromX("data/test.x", 0, device, 0, 0, 0, 0, &mesh2));

		eff     = load_effect(device, "data/test.fx");
		blur_fx = load_effect(device, "data/blur.fx");
		blit_fx = load_effect(device, "data/blit.fx");
		tex_fx  = load_effect(device, "data/tex.fx");
		brightpass_fx = load_effect(device, "data/brightpass.fx");

		d3d_err(D3DXCreateTextureFromFile(device, "data/map.tga", &tex));

		CComPtr<IDirect3DCubeTexture9> env;
		d3d_err(D3DXCreateCubeTextureFromFile(device, "data/stpeters_cross.dds", &env));
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

		device->BeginScene();

		// setup projection
		D3DXMATRIX proj;

		D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(90.f), 3.f / 4, 1.f, 100000.f);
		device->SetTransform(D3DTS_PROJECTION, &proj);

		float rot = time;

		// setup camera (animate parameters)
		D3DXVECTOR3 at(0, 0.f, 10.f);
		D3DXVECTOR3 up(0.f, 0.f, 1.f);
		D3DXVECTOR3 eye(
			sin(rot * 0.25f) * 10,
			cos(rot * 0.25f) * 10,
			10.f);

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

		device->SetRenderTarget(0, rtex_surf);
		device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DXCOLOR(0.f, 0.f, 0.f, 0.f), 1.f, 0);
		eff.draw(mesh2);

//		device->StretchRect(rtex_surf, NULL, bloomtex_surf[0], NULL, D3DTEXF_NONE);
		device->SetRenderTarget(0, bloomtex_surf[0]);
		blit(device, rtex, brightpass_fx, polygon);

		for (unsigned i = 0; i < 3; ++i)
		{
			device->SetRenderTarget(0, bloomtex_surf[1]);
			D3DXVECTOR4 dir;
			dir = D3DXVECTOR4((float(i) * 0.5f + 1.f) / backbuffer.get_desc().Width, 0.f, 0.f, 0.f);
			blur_fx->SetVector("dir", &dir);
			blit(device, bloomtex[0], blur_fx, polygon);

			device->SetRenderTarget(0, bloomtex_surf[0]);
			dir = D3DXVECTOR4(0.f, (float(i) * 0.5f + 1.f) / backbuffer.get_desc().Height, 0.f, 0.f);
			blur_fx->SetVector("dir", &dir);
			blit(device, bloomtex[1], blur_fx, polygon);
		}

		device->SetRenderTarget(0, backbuffer.get_surface());
		device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0.f, 0.f, 0.f, 0.f), 1.f, 0);
		blit_fx->SetTexture("bloom", bloomtex[0]);
		blit(device, rtex, blit_fx, polygon);
//		blit(device, bloomtex[0], blit_fx, polygon);

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
