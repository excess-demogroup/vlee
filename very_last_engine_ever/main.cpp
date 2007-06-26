#include "stdafx.h"
#include "resource.h"
#include "config.h"
#include "configdialog.h"
#include "init.h"

#include "core/fatalexception.h"

#include "math/vector2.h"
#include "math/vector3.h"
#include "math/matrix4x4.h"
#include "renderer/device.h"
#include "renderer/surface.h"
#include "renderer/texture.h"
#include "renderer/rendertexture.h"
#include "engine/scenerender.h"
#include "engine/mesh.h"
#include "engine/effect.h"
#include "engine/image.h"
#include "engine/anim.h"

#include "engine/textureproxy.h"

using math::Vector2;
using math::Vector3;
using math::Matrix4x4;

using renderer::Device;
using renderer::Surface;
using renderer::Texture;
using renderer::RenderTexture;

using engine::Mesh;
using engine::Effect;
using engine::Image;
using engine::Anim;

Matrix4x4 radialblur_matrix(const Texture &tex, const Vector2 &center, const float amt = 1.01)
{
	Matrix4x4 trans1;
	trans1.make_identity();
	trans1._13 = 0.5;
	trans1._23 = 0.5;
	
	Matrix4x4 mat;
	mat.make_scaling(Vector3(amt, amt, 1));
	
	Matrix4x4 trans2;
	trans2.make_identity();
	trans2._13 = -0.5;
	trans2._23 = -0.5;
	
	return trans1 * mat * trans2;
}

Matrix4x4 texture_matrix(const Texture &tex)
{
	Matrix4x4 mat, trans1, trans2, scale;

	//	D3DXMatrixIdentity(&trans2);
//	trans1._31 = 0.5;
//	trans1._32 = 0.5;

	D3DXMatrixIdentity(&trans2);
	trans2._31 = -10.5f / tex.get_surface().get_desc().Width;
	trans2._32 = -10.5f / tex.get_surface().get_desc().Height;

	D3DXMatrixScaling(&scale, 0.5, 0.5, 1.0);
	return trans2;
//	mat = trans2 * scale;

//	mat = trans1 * scale;
	return mat;
}


void blit(IDirect3DDevice9 *device, IDirect3DTexture9 *tex, Effect &eff, float x, float y, float w, float h)
{
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	eff->SetTexture("tex", tex);

	float s_nudge = 0.0f, t_nudge = 0.0f;
	float x_nudge = 0.0f, y_nudge = 0.0f;

	/* get render target */
	IDirect3DSurface9 *rt;
	device->GetRenderTarget(0, &rt);
	
	/* get surface description */
	D3DSURFACE_DESC rt_desc;
	rt->GetDesc(&rt_desc);

	/* setup nudge */
	x_nudge = -0.5f / (float(rt_desc.Width)  / 2);
	y_nudge =  0.5f / (float(rt_desc.Height) / 2);

	/* get texture description */
	D3DSURFACE_DESC tex_desc;
	tex->GetLevelDesc(0, &tex_desc);

	/* setup nudge */
	s_nudge = 0.0f / tex_desc.Width;
	t_nudge = 0.0f / tex_desc.Height;

	UINT passes;
	eff->Begin(&passes, 0);
	for (unsigned j = 0; j < passes; ++j)
	{
		eff->BeginPass(j);
		float verts[] =
		{
			x+     x_nudge, y +     y_nudge, 0, 0 + s_nudge, 1 + t_nudge,
			x+ w + x_nudge, y +     y_nudge, 0, 1 + s_nudge, 1 + t_nudge,
			x+ w + x_nudge, y + h + y_nudge, 0, 1 + s_nudge, 0 + t_nudge,
			x+     x_nudge, y + h + y_nudge, 0, 0 + s_nudge, 0 + t_nudge,
		};
		
		device->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
		device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(float) * 5);
		eff->EndPass();
	}
	eff->End();
}

#include "sync/SyncEditor.h"

WTL::CAppModule _Module;

using namespace core;
using namespace scenegraph;

HWND win = 0;
HSTREAM stream = 0;

static float spectrum[256];

float randf()
{
	return rand() * (1.f / RAND_MAX);
}

template <typename T>
CComPtr<T> com_ptr(T *ptr)
{
	// make a CComPtr<T> without adding a reference
	CComPtr<T> com_ptr;
	com_ptr.Attach(ptr); // don't addref
	return com_ptr;
}

int main(int /*argc*/, char* /*argv*/ [])
{
#ifndef NDEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
	_CrtSetBreakAlloc(68);
#endif
	
	_Module.Init(NULL, GetModuleHandle(0));
	
	try
	{
		CComPtr<IDirect3D9> direct3d = com_ptr(Direct3DCreate9(D3D_SDK_VERSION));
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
		
		D3DDISPLAYMODE mode = config.getMode();
		win = CreateWindow("static", "very last engine ever", WS_POPUP, 0, 0, mode.Width, mode.Height, 0, 0, GetModuleHandle(0), 0);
		if (!win) throw FatalException("CreateWindow() failed. something is VERY spooky.");
		Device device;
		device.Attach(init::initD3D(direct3d, win, mode, config.getMultisample(), config.getAdapter(), config.getVsync()));
//		device.Attach(init_d3d(direct3d, win, mode, D3DMULTISAMPLE_NONE, config.get_adapter(), config.get_vsync()));
		ShowWindow(win, TRUE); // showing window after initing d3d in order to be able to see warnings during init
		
		if (!BASS_Init(config.getSoundcard(), 44100, BASS_DEVICE_LATENCY, 0, 0)) throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/elg.mp3", 0, 0, BASS_MP3_SETPOS | ((0 == config.getSoundcard()) ? BASS_STREAM_DECODE : 0));
		if (!stream) throw FatalException("failed to open tune");
		
		SyncTimerBASS_Stream synctimer(stream, BPM, 4);
		
#ifdef SYNC
		SyncEditor sync("data\\__data_%s_%s.sync", synctimer);
#else
		Sync sync("data\\__data_%s_%s.sync", synctimer);
#endif
		
#if !WINDOWED
		ShowCursor(0);
#endif
		
		Surface backbuffer;
		backbuffer.Attach(device.get_render_target(0)); /* trick the ref-counter */
		
#ifdef SYNC
		sync.showEditor();
#endif
		


		/** DEMO ***/

//		RenderTexture rt(device, 128, 128, 1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_4_SAMPLES);
		RenderTexture rt(device, config.getMode().Width, config.getMode().Height, 1, D3DFMT_A8R8G8B8, config.getMultisample());
		RenderTexture rt2(device, config.getMode().Width, config.getMode().Height, 1, D3DFMT_A8R8G8B8);
		RenderTexture rt3(device, config.getMode().Width, config.getMode().Height, 1, D3DFMT_A8R8G8B8);

		Effect tex_fx      = engine::loadEffect(device, "data/tex.fx");
		Effect blur_fx     = engine::loadEffect(device, "data/blur.fx");

		Image   rt_img(rt, tex_fx);

		Texture arrow_tex  = engine::loadTexture(device, "data/arrow.dds");
		Effect  arrow_fx   = engine::loadEffect(device, "data/arrow.fx");
		Image   arrow_img(arrow_tex, arrow_fx);

		Anim moose_anim = engine::loadAnim(device, "data/moose");

		Mesh    tunelle_mesh = engine::loadMesh(device, "data/tunelle.x");
		Texture tunelle_tex  = engine::loadTexture(device, "data/tunelle.dds");
		Effect  tunelle_fx   = engine::loadEffect(device, "data/tunelle.fx");
		
		
		BASS_Start();
		BASS_ChannelPlay(stream, false);
		BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, 0.0f) + 10);
		
		bool done = false;
		while (!done)
		{
#ifndef VJSYS
			if (!sync.doEvents()) done = true;
#endif

			static float last_time = 0.f;
			static float time_offset = 0.f;
			double time = BASS_ChannelBytes2Seconds(stream, BASS_ChannelGetPosition(stream));
			double beat = time * (double(BPM) / 60);

#ifndef VJSYS
			sync.update(); //gets current timing info from the SyncTimer.
#endif
			device->BeginScene();
			
			core::d3d_err(device->SetRenderTarget(0, rt));
			
			D3DXCOLOR clear_color(1,0,0,0);
			device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, clear_color, 1.f, 0);
			
			Vector3 eye = Vector3(0, 0, 0);
			Vector3 at  = Vector3(0, 0, 100);
			Vector3 up  = Vector3(0, 1, 0);

			D3DXMATRIX view;
			D3DXMatrixLookAtLH(&view, &eye, &at, &up);
			Matrix4x4 world;
			D3DXMatrixIdentity(&world);
			D3DXMATRIX proj;
			D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(90), 16.f / 9, 0.01f, 1000.f);


			Vector3 scale(0.1, 0.1, 0.1);
			Vector3 translation(0, 0, 0);

			D3DXQUATERNION rotation;
			D3DXQuaternionRotationYawPitchRoll(&rotation, 0, float(M_PI / 2), 0);
			D3DXQUATERNION rotation2;
			D3DXQuaternionRotationYawPitchRoll(&rotation2, 0, 0, time * 0.01f);

			rotation *= rotation2;


			D3DXMatrixTransformation(&world, NULL, NULL, &scale, NULL, &rotation, &translation);

			tunelle_fx.set_matrices(world, view, proj);
			tunelle_fx->SetFloatArray("fog_color", clear_color, 3);
			tunelle_fx->SetTexture("map", tunelle_tex);
			tunelle_fx->SetFloat("overbright", 1.0);

			tunelle_fx.draw(tunelle_mesh);

			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);

			float s = 1.0 / (1 + fmod(beat / 2, 1.0) * 0.25);

			tex_fx->SetFloat("xoffs", 1 - fmod(time * 0.25, 2));
			tex_fx->SetFloat("yoffs", 0.0f);
			tex_fx->SetFloat("xzoom", ((1.0f)    / 1.5) * s);
			tex_fx->SetFloat("yzoom", ((4.f / 3) / 1.5) * s);

			// w = ((1.0f)    / 1.5) * s;
			// h = ((4.f / 3) / 1.5) * s;

			{
				float w = ((1.0f)    / 0.75) * s;
				float h = ((4.f / 3) / 0.75) * s;
				blit(
					device,
					moose_anim.getFramePingPong(float(beat / 8)),
					tex_fx,
					1 - fmod(time * 0.5, 2) - w / 2,
					0.0f  - h / 2,
					w, h
				);
			}

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			
			core::d3d_err(device->SetRenderTarget(0, rt2));
			rt.resolve();
//			device->StretchRect(test_surf, NULL, test_surf2, NULL, D3DTEXF_NONE);
//			device->StretchRect(rt.get_surface(0), NULL, backbuffer, NULL, D3DTEXF_POINT);

			device->Clear(0, 0, D3DCLEAR_TARGET, clear_color, 1.f, 0);

			rt_img.x = -1;
			rt_img.y = -1;
			rt_img.w =  2;
			rt_img.h =  2;
//			rt_img.draw(device);

			Matrix4x4 texcoord_transform;
			Matrix4x4 texture_transform = texture_matrix(rt);

			Vector3 texcoord_translate(
				0.5 + (0.5 / rt.get_surface().get_desc().Width),
				0.5 + (0.5 / rt.get_surface().get_desc().Height),
				0.0);
			blur_fx->SetFloatArray("texcoord_translate", texcoord_translate, 2);
			texcoord_transform.make_scaling(Vector3(1,1,1));
			blur_fx->SetMatrix("texcoord_transform", &texcoord_transform);
			blur_fx->SetMatrix("texture_transform", &texture_transform);

//			device->SetRenderTarget(0, blurme2_tex.get_surface());
			float amt = 1.0 / (1 + ((1 - fmod(time, 1.0)) * 0.02f));
			Matrix4x4 texel_transform = radialblur_matrix(rt, Vector2(0, 0), amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);

			blit(device, rt, blur_fx, -1, -1, 2, 2);
//			blit(device, blurme1_tex, blur_fx, polygon);

			amt = 1.0 / (1 + ((1 - fmod(time, 1.0)) * 0.04f));
			texel_transform = radialblur_matrix(rt, Vector2(0, 0), amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);

			core::d3d_err(device->SetRenderTarget(0, rt3));
			device->Clear(0, 0, D3DCLEAR_TARGET, clear_color, 1.f, 0);
			blit(device, rt2, blur_fx, -1, -1, 2, 2);

			amt = 1.0 / (1 + ((1 - fmod(time, 1.0)) * 0.08f));
			texel_transform = radialblur_matrix(rt, Vector2(0, 0), amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);

			core::d3d_err(device->SetRenderTarget(0, rt2));
			device->Clear(0, 0, D3DCLEAR_TARGET, clear_color, 1.f, 0);
			blit(device, rt3, blur_fx, -1, -1, 2, 2);

			amt = 1.0 / (1 + ((1 - fmod(time, 1.0)) * 0.16f));
			texel_transform = radialblur_matrix(rt, Vector2(0, 0), amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);

			core::d3d_err(device->SetRenderTarget(0, backbuffer));
			device->Clear(0, 0, D3DCLEAR_TARGET, clear_color, 1.f, 0);
			blit(device, rt2, blur_fx, -1, -1, 2, 2);

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
			s = 1.0 / (1 + fmod(beat, 1.0));
			arrow_img.w = ((1.0f)    / 3) * s;
			arrow_img.h = ((4.f / 3) / 3) * s;

			arrow_img.x = 1 - fmod(time * 0.5, 2) - arrow_img.w / 2;
			arrow_img.y = 0.85f  - arrow_img.h / 2;
			arrow_fx->SetFloat("time", time);
			arrow_img.draw(device);

			arrow_img.w = -((1.0f)    / 3) * s;
			arrow_img.x = 1 - fmod((time + 0.5) * 0.5, 2) - arrow_img.w / 2;
			arrow_img.y = 0.6f - arrow_img.h / 2;
			arrow_img.draw(device);

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);


#if 0
			test_img.x = -1;
			test_img.y = -1;
//			test_img.x = -1 - 1.0 / config.get_mode().Width;
//			test_img.y = -1 + 1.0 / config.get_mode().Height;

			test_img.w =  2;
			test_img.h =  2;
			test_img.draw(device);
#endif
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




		/** END OF DEMO ***/








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
