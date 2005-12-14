#include "stdafx.h"
#include "resource.h"
#include "config.h"
#include "configdialog.h"
#include "init.h"

#include "sync/SyncEditor.h"

#include "engine/engine.h"

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

void set_ramp(float alphas[], float base)
{
	for (unsigned i = 0; i < 8; i++)
	{
		alphas[i] = base;
		base *= 0.5f;
	}
}

CComPtr<IDirect3DTexture9> rtex[2];
CComPtr<IDirect3DSurface9> rtex_surf[2];

Mesh polygon;
Effect blur_fx;
Effect tex_fx;
Effect tex_blend_fx;

void draw_tex(IDirect3DDevice9 *device, IDirect3DTexture9 *tex, float alpha = 1.f, float xoffs = 0.f, float yoffs = 0.f, float zoom = 1.f, float zoom2 = 1.f)
{
	assert(polygon);
	assert(tex_fx);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);


	tex_fx->SetTexture("tex", tex);
	tex_fx->SetFloat("alpha", alpha);
	tex_fx->SetFloat("xoffs", xoffs);
	tex_fx->SetFloat("yoffs", yoffs);

	tex_fx->SetFloat("xzoom", zoom);
	tex_fx->SetFloat("yzoom", zoom2);

	tex_fx->CommitChanges();

	tex_fx.draw(polygon);
}

void draw_tex_blend(IDirect3DDevice9 *device, IDirect3DTexture9 *tex1, IDirect3DTexture9 *tex2, float alpha)
{
	assert(polygon);
	assert(tex_blend_fx);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	tex_blend_fx->SetTexture("tex1", tex1);
	tex_blend_fx->SetTexture("tex2", tex2);
	tex_blend_fx->SetFloat("alpha", alpha);

	tex_blend_fx->CommitChanges();
	tex_blend_fx.draw(polygon);
}

void draw_stroke(IDirect3DDevice9 *device, float dir, IDirect3DTexture9 *tex)
{
	assert(blur_fx);
	assert(polygon);

	float alpha[8];
	float dir_vec[2];

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	for (unsigned i = 0; i < 3; i++)
	{
		device->SetRenderTarget(0, rtex_surf[i & 1]);

		if (i == 0) blur_fx->SetTexture("blur_tex", tex);
		else blur_fx->SetTexture("blur_tex", rtex[(i - 1) & 1]);

		dir_vec[0] = cosf(dir) * (float(1 << i) / 256);
		dir_vec[1] = sinf(dir) * (float(1 << i) / 256);
		blur_fx->SetFloatArray("dir", dir_vec, 2);
		set_ramp(alpha, 0.5f);
		blur_fx->SetFloatArray("alpha", alpha, 8);

		blur_fx->CommitChanges();
		blur_fx.draw(polygon);
	}
}

void draw_blur(IDirect3DDevice9 *device, float dir, IDirect3DTexture9 *tex)
{
	assert(blur_fx);
	assert(polygon);

	float alpha[8] = { 0.03f, 0.075f, 0.15f, 0.3f, 0.3f, 0.15f, 0.075f, 0.03f };
	float dir_vec[2];

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	for (unsigned i = 0; i < 2; i++)
	{
		device->SetRenderTarget(0, rtex_surf[i & 1]);

		if (i == 0) blur_fx->SetTexture("blur_tex", tex);
		else blur_fx->SetTexture("blur_tex", rtex[(i - 1) & 1]);

		dir_vec[0] = cosf(dir) * (float(1 << i) / 256);
		dir_vec[1] = sinf(dir) * (float(1 << i) / 256);
		dir_vec[0] *= 3.f / 4.f;
		blur_fx->SetFloatArray("dir", dir_vec, 2);
		blur_fx->SetFloatArray("alpha", alpha, 8);

		blur_fx->CommitChanges();
		blur_fx.draw(polygon);
	}
}

float offs(float t)
{
	if (t > 4) floor(t * (int(t > 8) + 1) * (int(t > 32) + 1));
	return 0.f;
}

// void megaman

int main(int /*argc*/, char* /*argv*/ [])
{

#ifndef NDEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
//	_CrtSetBreakAlloc(65);
#endif

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
		SyncTimerBASS_Stream synctimer(stream, 145, 4);

#if defined(_DEBUG)
		SyncEditor sync("data\\__data_%s_%s.sync", synctimer);
#else
		Sync sync("data\\__data_%s_%s.sync", synctimer);
#endif

		SyncTrack& cam_dist = sync.getTrack("dist", "cam", 5, true);
		SyncTrack& cam_fov = sync.getTrack("fov", "cam", 5, true);
		SyncTrack& global_fade = sync.getTrack("fade", "global", 5, true);

		SyncTrack& global_glowfade = sync.getTrack("glowfade", "global", 5, true);
		SyncTrack& global_objfade  = sync.getTrack("objfade", "global", 5, true);
		SyncTrack& global_ovalaifade  = sync.getTrack("ovalaifade", "global", 5, true);

		SyncTrack& global_zoom = sync.getTrack("zoom", "global", 5, true);
		SyncTrack& global_zoom2 = sync.getTrack("zoom2", "global", 5, true);

		SyncTrack& global_feedback = sync.getTrack("feedback", "global", 5, true);

		SyncTrack& time_bias = sync.getTrack("time bias", "global", 5, true);

#if !WINDOWED
		ShowCursor(0);
#endif

		Surface backbuffer = Surface::get_render_target(device);

		d3d_err(device->CreateTexture(256, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &rtex[0], 0));
		d3d_err(device->CreateTexture(256, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &rtex[1], 0));
		d3d_err(rtex[0]->GetSurfaceLevel(0, &rtex_surf[0]));
		d3d_err(rtex[1]->GetSurfaceLevel(0, &rtex_surf[1]));

		CComPtr<IDirect3DTexture9> rtex2;
		CComPtr<IDirect3DSurface9> rtex2_surf;
		d3d_err(device->CreateTexture(256, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &rtex2, 0));
		d3d_err(rtex2->GetSurfaceLevel(0, &rtex2_surf));

		CComPtr<IDirect3DTexture9> rtex3;
		CComPtr<IDirect3DSurface9> rtex3_surf;
		d3d_err(device->CreateTexture(256, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &rtex3, 0));
		d3d_err(rtex3->GetSurfaceLevel(0, &rtex3_surf));

		CComPtr<IDirect3DTexture9> rtex4;
		CComPtr<IDirect3DSurface9> rtex4_surf;
		d3d_err(device->CreateTexture(256, 256, 0, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &rtex4, 0));
		d3d_err(rtex4->GetSurfaceLevel(0, &rtex4_surf));

		d3d_err(D3DXCreatePolygon(device, 3.f, 4, &polygon, 0));

		Mesh mesh2;
		d3d_err(D3DXLoadMeshFromX("data/fjallblob2k2.x", 0, device, 0, 0, 0, 0, &mesh2));

		Effect eff;
		d3d_err(D3DXCreateEffectFromFile(device, "data/test.fx", NULL, NULL, 0, NULL, &eff, NULL));
		eff.update();

		Effect eff2;
		d3d_err(D3DXCreateEffectFromFile(device, "data/test2.fx", NULL, NULL, 0, NULL, &eff2, NULL));
		eff2.update();

		d3d_err(D3DXCreateEffectFromFile(device, "data/blur.fx", NULL, NULL, 0, NULL, &blur_fx, NULL));
		d3d_err(D3DXCreateEffectFromFile(device, "data/tex.fx", NULL, NULL, 0, NULL, &tex_fx, NULL));
		d3d_err(D3DXCreateEffectFromFile(device, "data/tex_blend.fx", NULL, NULL, 0, NULL, &tex_blend_fx, NULL));

		CComPtr<IDirect3DCubeTexture9> tex;
		d3d_err(D3DXCreateCubeTextureFromFile(device, "data/stpeters_cross3.dds", &tex));

		CComPtr<IDirect3DCubeTexture9> tex2;
		d3d_err(D3DXCreateCubeTextureFromFile(device, "data/stpeters_cross3.dds", &tex2));

		CComPtr<IDirect3DTexture9> kjossmae[4];
		d3d_err(D3DXCreateTextureFromFile(device, "data/kjossmae.png", &kjossmae[0]));
		d3d_err(D3DXCreateTextureFromFile(device, "data/kjossmae2.png", &kjossmae[1]));
		d3d_err(D3DXCreateTextureFromFile(device, "data/kjossmae3.png", &kjossmae[2]));
		d3d_err(D3DXCreateTextureFromFile(device, "data/kjossmae4.png", &kjossmae[3]));

		CComPtr<IDirect3DTexture9> logo;
		d3d_err(D3DXCreateTextureFromFile(device, "data/logo.png", &logo));

		BASS_Start();
		BASS_ChannelPlay(stream, false);
//		BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, 60.0f) + 10);

		sync.showEditor();

		bool done = false;
		while (!done)
		{
			sync.update(); //gets current timing info from the SyncTimer.

			static float last_time = 0.f;
			static float time_offset = 0.f;
			float time = BASS_ChannelBytes2Seconds(stream, BASS_ChannelGetPosition(stream));
			float dtime = time - last_time;
			last_time = time;
			float real_time = time;
			time += time_offset + time_bias.getFloatValue();

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

			static float last_avg_level = 0.f;
			float avg_level = last_avg_level + (spectrum[0] - last_avg_level) * 0.3f;
			last_avg_level = avg_level;

			static float last_avg_levels = 0.f;
			float avg_levels = last_avg_levels + (spectrum[0] - last_avg_levels) * 0.001f;
			last_avg_levels = avg_levels;

			if ((spectrum[0] - avg_level) > 0.1f) time_offset += 0.1f + randf();

			static float shit = 0.f;
			if (shit < 0.1f) shit += spectrum[0] * dtime;
			shit *= 0.98f;

			float dark = 1.0f;
			if (real_time > 39.5 && real_time < 43.1) dark = 0.5f;
			if (real_time > 46.5 && real_time < 49.4) dark = 0.5f;
			if (real_time > 52.5 && real_time < 56.4) dark = 0.5f;
			if (real_time > 59.3 && real_time < 62.9) dark = 0.5f;

			if (real_time > 65.8 && real_time < 66.2)
			{
				dark = 0.0f;
				time_offset += 3.0f;
			}

			// setup projection
			D3DXMATRIX proj;
			D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(cam_fov.getFloatValue() + shit * 300 - 10), 4.f / 3, 1.f, 100000.f);
			device->SetTransform(D3DTS_PROJECTION, &proj);

			float amt = spectrum[1] * 10 * dark;
			float dist = 40.f - shit * 10;
			time += offs(time);

			dist = cam_dist.getFloatValue();

			// setup camera (animate parameters)
			D3DXVECTOR3 at(0, 0.f, 0);
			D3DXVECTOR3 up(0.f, 1.f, 0.f);
			D3DXVECTOR3 eye(
				sin(time * 0.25f) * dist + sin(cos(time * 30)) * amt,
				cos(time * 0.25f) * dist + sin(sin(time * 30)) * amt,
				cos(time * 0.25f) * dist + cos(sin(time * 30)) * amt);

			// setup camera (view matrix)
			D3DXMATRIX view;
			D3DXMatrixLookAtLH(&view, &eye, &at, &up);
			device->SetTransform(D3DTS_VIEW, &view);

			// setup object matrix
			D3DXMATRIX world;
			D3DXMatrixIdentity(&world);
			device->SetTransform(D3DTS_WORLD, &world);

			eff.set_matrices(world, view, proj);
			eff->SetTexture("EnvironmentMap", tex);
			eff->CommitChanges();

			device->SetRenderTarget(0, rtex_surf[1]);
			device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DXCOLOR(0.f, 0.f, 0.f, 0.f), 1.f, 0);

			eff->SetFloat("fade", global_fade.getFloatValue() * (1.f / 256));
			eff.draw(mesh2);

			draw_blur(device, 0.f, rtex[1]);
			draw_blur(device, float(M_PI / 2), rtex[1]);

			device->SetRenderTarget(0, rtex2_surf);
			draw_tex(device, rtex[1], 0.25f + spectrum[10] * 100 * global_glowfade.getFloatValue() * (1.f / 256));

			device->SetRenderTarget(0, backbuffer.get_surface());
			device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DXCOLOR(0.f, 0.f, 0.f, 0.f), 1.f, 0);

			eff->SetTexture("EnvironmentMap", tex2);
			eff->SetFloat("fade", global_fade.getFloatValue() * (1.f / 256) * global_objfade.getFloatValue() * (1.f / 256));
			eff->CommitChanges();
			eff.draw(mesh2);

			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);

			draw_tex(device, rtex2);

			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);

			if (spectrum[0] > avg_level + 0.05f) draw_tex(device, kjossmae[int(randf() * randf() * 8) % 4], 0.25f * global_ovalaifade.getFloatValue() * (1.f / 256), randf() > 0.95f ? 0.2f : 0.f, randf() > 0.1f ? 0.0f : 0.05f);
			if (spectrum[0] > avg_level + 0.025f) draw_tex(device, kjossmae[int(randf() * randf() * 8) % 4], 1.f * global_ovalaifade.getFloatValue() * (1.f / 256), randf() > 0.95f ? 0.2f : 0.f, randf() > 0.1f ? 0.0f : 0.05f);


			static float last_avg_level5 = 0.f;
			float avg_level5 = last_avg_level5 + (spectrum[5] - last_avg_level5) * 0.99f;
			last_avg_level5 = avg_level5;

//			draw_tex(device, rtex4, 30.9f * avg_level5 - avg_levels, sin(time - cos(time * 3)) * 0.01f, cos(time - cos(time * 3)) * 0.01f, global_zoom.getFloatValue() * (1.f / 256));
//			                 tex,   alpha, float xoffs = 0.f, float yoffs = 0.f, float zoom = 1.f, float zoom2 = 1.f

			draw_tex(device, rtex4, global_feedback.getFloatValue() * (1.f / 256), sin(time - cos(time * 3)) * 0.01f, cos(time - cos(time * 3)) * 0.01f, global_zoom.getFloatValue() * (1.f / 256), global_zoom2.getFloatValue() * (1.f / 256));
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

			device->StretchRect(backbuffer.get_surface(), NULL, rtex4_surf, NULL, D3DTEXF_LINEAR);

			if ((real_time) > (141.9f - 1))
			{
				device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
				draw_tex(device, logo, (real_time - (141.9f - 1)) * spectrum[0] * 100);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			}

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
					if (VK_SPACE == LOWORD(msg.wParam)) ::printf("***** time: %f\n", real_time);
				}
			}
			if (!sync.doEvents()) done = true;
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

	return 0;
}
