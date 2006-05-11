#include "stdafx.h"
#include "resource.h"
#include "config.h"
#include "configdialog.h"
#include "init.h"
#include "engine/engine.h"

WTL::CAppModule _Module;

using engine::core::FatalException;
using engine::core::Surface;
using engine::core::d3d_err;
using namespace engine::core;

using engine::Mesh;
using engine::Effect;

// using namespace engine::core;
using namespace engine::scenegraph;

HWND win = 0;
HSTREAM stream = 0;

static float spectrum[256];

float randf()
{
	return rand() * (1.f / RAND_MAX);
}

template <typename T>
CComPtr<T> d3d_ptr(T *ptr) {
	// make a CComPtr<T> without adding a reference
	CComPtr<T> com_ptr;
	com_ptr.Attach(ptr); // don't addref
	return com_ptr;
}

Effect compile_effect(IDirect3DDevice9 *device, const char * filename)
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

int main(int /*argc*/, char* /*argv*/ [])
{

#ifndef NDEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
//	_CrtSetBreakAlloc(68);
#endif

	_Module.Init(NULL, GetModuleHandle(0));

	try
	{
		CComPtr<IDirect3D9> direct3d = d3d_ptr(Direct3DCreate9(D3D_SDK_VERSION));
		if (!direct3d) throw FatalException("your directx-version is from the stone-age.\n\nTHRUG SAYS: UPGRADE!");

		ConfigDialog config(direct3d);

#ifdef NDEBUG
		INT_PTR result = config.DoModal();
		if (FAILED(result)) MessageBox(NULL, "could not initialize dialogbox, using default settings", NULL, MB_OK);
		else
		{
			if (IDOK != result)
			{
				// cancel was hit...
				MessageBox(NULL, "damn whimp...", "pfff", MB_OK);
				return 0;
			}
		}
#endif

		D3DDISPLAYMODE mode = config.get_mode();
		win = CreateWindow("static", "very last engine ever", WS_POPUP, 0, 0, mode.Width, mode.Height, 0, 0, GetModuleHandle(0), 0);
		if (!win) throw FatalException("CreateWindow() failed. something is VERY spooky.");

		CComPtr<IDirect3DDevice9> device;
		device.Attach(init_d3d(direct3d, win, mode, config.get_multisample(), config.get_adapter(), config.get_vsync()));

		ShowWindow(win, TRUE); // showing window after initing d3d in order to be able to see warnings during init

#ifndef VJSYS
		if (!BASS_Init(config.get_soundcard(), 44100, BASS_DEVICE_LATENCY, 0, 0)) throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/irvin_-_phyllis_still_craves_for_it.ogg", 0, 0, BASS_MP3_SETPOS | ((0 == config.get_soundcard()) ? BASS_STREAM_DECODE : 0));
		if (!stream) throw FatalException("failed to open tune");
#else
		BASS_RecordInit(config.get_soundcard());
		HRECORD stream = BASS_RecordStart(44100, 2, BASS_RECORD_PAUSE, 0, 0);
		if (!stream) throw FatalException("failed to open input-device");
#endif



#if !WINDOWED
		ShowCursor(0);
#endif

		Surface backbuffer = Surface::get_render_target(device);

		Texture rtex(device, config.get_mode().Width, config.get_mode().Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, 0);
		CComPtr<IDirect3DSurface9> rtex_surf = rtex.get_surface(0);

		Texture bloomtex[2];
		bloomtex[0] = Texture(device, config.get_mode().Width, config.get_mode().Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, 0);
		bloomtex[1] = Texture(device, config.get_mode().Width, config.get_mode().Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, 0);

		CComPtr<IDirect3DSurface9> bloomtex_surf[2];
		bloomtex_surf[0] = bloomtex[0].get_surface(0);
		bloomtex_surf[1] = bloomtex[1].get_surface(0);

		Mesh polygon;
		d3d_err(D3DXCreatePolygon(device, 3.f, 4, &polygon, 0));

		Mesh mesh2;
		d3d_err(D3DXLoadMeshFromX("data/test.x", 0, device, 0, 0, 0, 0, &mesh2));

		Effect eff = compile_effect(device, "data/test.fx");
		Effect blur_fx = compile_effect(device, "data/blur.fx");
		Effect blit_fx = compile_effect(device, "data/blit.fx");
		Effect brightpass_fx = compile_effect(device, "data/brightpass.fx");

		Effect tex_fx = compile_effect(device, "data/tex.fx");

		Texture tex;
		d3d_err(D3DXCreateTextureFromFile(device, "data/map.tga", &tex));

		CComPtr<IDirect3DCubeTexture9> env;
		d3d_err(D3DXCreateCubeTextureFromFile(device, "data/stpeters_cross.dds", &env));
		eff->SetTexture("env", env);


		BASS_Start();
		BASS_ChannelPlay(stream, false);
//		BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, 60.0f) + 10);

		bool done = false;
		while (!done)
		{

			static float last_time = 0.f;
			static float time_offset = 0.f;
			float time = BASS_ChannelBytes2Seconds(stream, BASS_ChannelGetPosition(stream));

#ifndef NDEBUG
//			printf("real_time: %f\n", real_time);
#endif

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

			D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(90.f), 4.f / 3, 1.f, 100000.f);
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

//			device->StretchRect(rtex_surf, NULL, bloomtex_surf[0], NULL, D3DTEXF_NONE);
			device->SetRenderTarget(0, bloomtex_surf[0]);
			blit(device, rtex, brightpass_fx, polygon);

			for (unsigned i = 0; i < 3; ++i)
			{
				device->SetRenderTarget(0, bloomtex_surf[1]);
				D3DXVECTOR4 dir;
				dir = D3DXVECTOR4((float(i) * 0.5f + 1.f) / config.get_mode().Width, 0.f, 0.f, 0.f);
				blur_fx->SetVector("dir", &dir);
				blit(device, bloomtex[0], blur_fx, polygon);

				device->SetRenderTarget(0, bloomtex_surf[0]);
				dir = D3DXVECTOR4(0.f, (float(i) * 0.5f + 1.f) / config.get_mode().Height, 0.f, 0.f);
				blur_fx->SetVector("dir", &dir);
				blit(device, bloomtex[1], blur_fx, polygon);
			}

			device->SetRenderTarget(0, backbuffer.get_surface());
			device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0.f, 0.f, 0.f, 0.f), 1.f, 0);
			blit_fx->SetTexture("bloom", bloomtex[0]);
			blit(device, rtex, blit_fx, polygon);
//			blit(device, bloomtex[0], blit_fx, polygon);

			device->EndScene();
			HRESULT res = device->Present(0, 0, 0, 0);
			if (FAILED(res))
			{
				throw FatalException(std::string(DXGetErrorString9(res)) + std::string(" : ") + std::string(DXGetErrorDescription9(res)));
			}
			BASS_Update(); // decrease the chance of missing vsync

			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if (WM_QUIT == msg.message) done = true;
				if (WM_KEYDOWN == msg.message)
				{
					if (VK_ESCAPE == LOWORD(msg.wParam)) done = true;
				}
			}
		}

		// cleanup
		if (stream) BASS_StreamFree(stream);
		BASS_Free();
		if (win) DestroyWindow(win);
#if !WINDOWED
		ShowCursor(TRUE);
#endif
	
	}
	catch (const std::exception &e)
	{

		// cleanup
		if (stream) BASS_StreamFree(stream);
		BASS_Free();
		if (win) DestroyWindow(win);
#if !WINDOWED
		ShowCursor(TRUE);
#endif

		log::printf("\n*** error : %s\n", e.what());
		log::save("errorlog.txt");
		MessageBox(0, e.what(), 0, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
		return 1;
	}

	_Module.Term();
	return 0;
}
