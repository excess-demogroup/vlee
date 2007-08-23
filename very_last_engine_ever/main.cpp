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
	trans1._13 = 0.5f - center.x / 2;
	trans1._23 = 0.5f - center.y / 2;
	
	Matrix4x4 mat;
	mat.make_scaling(Vector3(amt, amt, 1));
	
	Matrix4x4 trans2;
	trans2.make_identity();
	trans2._13 = -(0.5f - center.x / 2);
	trans2._23 = -(0.5f - center.y / 2);
	
	return trans1 * mat * trans2;
}

Matrix4x4 texture_matrix(const Texture &tex)
{
	Matrix4x4 mat, trans1, trans2, scale;

	//	D3DXMatrixIdentity(&trans2);
//	trans1._31 = 0.5;
//	trans1._32 = 0.5;

	D3DXMatrixIdentity(&trans2);
	trans2._31 = -10.5f / tex.getSurface().getDesc().Width;
	trans2._32 = -10.5f / tex.getSurface().getDesc().Height;

	D3DXMatrixScaling(&scale, 0.5, 0.5, 1.0);
	return trans2;
//	mat = trans2 * scale;

//	mat = trans1 * scale;
	return mat;
}


void blit(renderer::Device &device, renderer::Texture &tex, Effect &eff, float x, float y, float w, float h)
{
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	eff->SetTexture("tex", tex);

	float s_nudge = 0.0f, t_nudge = 0.0f;
	float x_nudge = 0.0f, y_nudge = 0.0f;

	/* get render target */
	Surface rt = device.getRenderTarget(0);
	
	/* get surface description */
	D3DSURFACE_DESC rt_desc = rt.getDesc();

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
#if 0
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
#endif
		
		D3DDISPLAYMODE mode = config.getMode();
		win = CreateWindow("static", "very last engine ever", WS_POPUP, 0, 0, mode.Width, mode.Height, 0, 0, GetModuleHandle(0), 0);
		if (!win) throw FatalException("CreateWindow() failed. something is VERY spooky.");

		Device device;
		device.Attach(init::initD3D(direct3d, win, mode, D3DMULTISAMPLE_NONE, config.getAdapter(), config.getVsync()));
		ShowWindow(win, TRUE); // showing window after initing d3d in order to be able to see warnings during init
#if !WINDOWED
		ShowCursor(0);
#endif
		
		if (!BASS_Init(config.getSoundcard(), 44100, BASS_DEVICE_LATENCY, 0, 0)) throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/208_skritt_til_venstre.mp3", 0, 0, BASS_MP3_SETPOS | ((0 == config.getSoundcard()) ? BASS_STREAM_DECODE : 0));
		if (!stream) throw FatalException("failed to open tune");
		
		SyncTimerBASS_Stream synctimer(stream, BPM, 4);
		
#ifdef SYNC
		SyncEditor sync("data\\__data_%s_%s.sync", synctimer);
#else
		Sync sync("data\\__data_%s_%s.sync", synctimer);
#endif
		
		Surface backbuffer   = device.getRenderTarget(0); /* trick the ref-counter */
		Surface depthstencil = device.getDepthStencilSurface(); /* trick the ref-counter */
		
		RenderTexture color_msaa(device, config.getWidth(), config.getHeight(), 1, D3DFMT_A8R8G8B8, config.getMultisample());
		Surface depthstencil_msaa = device.createDepthStencilSurface(config.getWidth(), config.getHeight(), D3DFMT_D24S8, config.getMultisample());

		/** DEMO ***/

//		RenderTexture rt(device, 128, 128, 1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE);
		RenderTexture rt2(device, config.getWidth(), config.getHeight(), 1, D3DFMT_A8R8G8B8);
		RenderTexture rt3(device, config.getWidth(), config.getHeight(), 1, D3DFMT_A8R8G8B8);

/*
		Surface rt_ds = device.createDepthStencilSurface(config.getWidth(), config.getHeight(), D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE);
		RenderTexture pixelize(device, config.getWidth(), config.getHeight(), 1, D3DFMT_A8R8G8B8, config.getMultisample());
*/
		Matrix4x4 tex_transform;
		tex_transform.make_identity();
		Effect tex_fx      = engine::loadEffect(device, "data/tex.fx");
		tex_fx->SetMatrix("transform", &tex_transform);
		Effect blur_fx     = engine::loadEffect(device, "data/blur.fx");

		Effect pixelize_fx      = engine::loadEffect(device, "data/pixelize.fx");
		pixelize_fx->SetMatrix("transform", &tex_transform);
		pixelize_fx->SetMatrix("tex_transform", &tex_transform);

		Texture arrow_tex  = engine::loadTexture(device, "data/arrow.dds");
		Effect  arrow_fx   = engine::loadEffect(device, "data/arrow.fx");
		Image   arrow_img(arrow_tex, arrow_fx);
		Image   arrow_holder_img(engine::loadTexture(device, "data/arrow_holder.dds"), tex_fx);
		Image   solnedgang_img(engine::loadTexture(device, "data/solnedgang.dds"), tex_fx);

		/* nice blue sky */
		Effect  himmel_fx = engine::loadEffect(device, "data/himmel.fx");
		himmel_fx->SetTexture("disco_tex", engine::loadTexture(device, "data/beam.dds"));
		Image   himmel_img(engine::loadTexture(device, "data/himmel.dds"), himmel_fx);

		Image   sol_img(engine::loadTexture(device, "data/sol.dds"), tex_fx);
		Image   fjell_img(engine::loadTexture(device, "data/fjell.dds"), tex_fx);

		Image   vers_1_img(engine::loadTexture(device, "data/vers_1.dds"), tex_fx);
		Image   vers_2_img(engine::loadTexture(device, "data/vers_2.dds"), tex_fx);
		Image   refreng_1_img(engine::loadTexture(device, "data/refreng_1.dds"), tex_fx);
		Image   refreng_2_img(engine::loadTexture(device, "data/refreng_2.dds"), tex_fx);

		Image   blomst_01_img(engine::loadTexture(device, "data/blomst_01.dds"), tex_fx);
		Image   blomst_02_img(engine::loadTexture(device, "data/blomst_02.dds"), tex_fx);
		Image   blomst_03_img(engine::loadTexture(device, "data/blomst_03.dds"), tex_fx);

		Anim moose_anim = engine::loadAnim(device, "data/moose");

		BASS_Start();
		BASS_ChannelPlay(stream, false);
		BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, 0.0f) + 10);


		enum moose_state {
			LEFT, RIGHT, IDLE
		} ms = IDLE;
		float move_time = 0.0f;
		int pos = 0;

#ifdef SYNC
		sync.showEditor();
#endif


		bool done = false;
		while (!done)
		{
#ifndef VJSYS
			if (!sync.doEvents()) done = true;
#endif

			static float last_time = 0.f;
			static float time_offset = 0.f;
			float time = BASS_ChannelBytes2Seconds(stream, BASS_ChannelGetPosition(stream));
			float beat = time * (float(BPM) / 60);


			switch (ms)
			{
			case LEFT:
			case RIGHT:
				/* if time passed, go idle */
				if (beat - move_time > 1.0f) ms = IDLE;
			break;

			case IDLE:
				break;
			}
			
#ifndef VJSYS
			sync.update(); //gets current timing info from the SyncTimer.
#endif
			device->BeginScene();
			
			/* setup multisampled stuffz */
			device.setRenderTarget(color_msaa.getRenderTarget());
			device.setDepthStencilSurface(depthstencil_msaa);
			
			D3DVIEWPORT9 viewport;
			device->GetViewport(&viewport);
			viewport.Width = 32;
			viewport.Height = 32;
			device->SetViewport(&viewport);
			
			D3DXCOLOR clear_color(1,0,0,0);
			device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, clear_color, 1.f, 0);
			
/*
			solnedgang_img.x = -1;
			solnedgang_img.y = -1;
			solnedgang_img.w = 2;
			solnedgang_img.h = 2;
			solnedgang_img.draw(device);
*/
			
			D3DXVECTOR4 disco_offset(time, 0, 0,0);
			himmel_fx->SetVector("disco_offset", &disco_offset);
			himmel_fx->SetFloat("alpha", 1.0f);
			himmel_img.x = -1;
			himmel_img.y = -1;
			himmel_img.w = 2;
			himmel_img.h = 2;
			himmel_img.draw(device);

			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);

			float sol_boost = 1 + (1.0f / (1.0f + fmod((beat - 0) / 4, 1.0f))) * 0.25f;
			{
				Matrix4x4 temp, temp2;
				temp.make_scaling(Vector3(1.0 * sol_boost, (4.f/3) * sol_boost, 1.0 * sol_boost));
				temp2.make_translation(Vector3(-0.175f, 0.1f, 0.0f));
				tex_transform.make_rotation(Vector3(0, 0, time / 64));
				tex_transform = tex_transform * temp * temp2;
			}
			tex_fx->SetMatrix("transform", &tex_transform);

			sol_img.w = 1;
			sol_img.h = 1;
			sol_img.x = -0.5;
			sol_img.y = -0.5;
			sol_img.draw(device);

			tex_transform.make_identity();
			tex_fx->SetMatrix("transform", &tex_transform);

			fjell_img.x = -1;
			fjell_img.y = -1;
			fjell_img.w = 2;
			fjell_img.h = 2 + 0.25f / (1 + fmod(beat / 4, 1.0f));
			fjell_img.draw(device);
/*
			tex_fx->SetFloat("xoffs", 1 - fmod(beat * 0.25, 2));
			tex_fx->SetFloat("yoffs", 0.0f);
*/
			{
				float pulse = 0.0f;
				float s = 1.0f / (1 + fmod(beat / 2, 1.0f) * 0.25f * pulse);
				
				float w = ((1.0f)    / 0.8f) * s;
				float h = ((4.f / 3) / 0.8f) * s;
				int frame = 0;
				
				switch (ms)
				{
				case LEFT:
					frame = 0;
				break;
				
				case RIGHT:
					frame = 2;
				break;
				
				case IDLE:
					frame = 1;
				break;
				
				default: assert(0);
				}

//				int max_moose_level = 1 + (int(time) % 10); // moose_amt.getIntValue();

				int max_moose_count = 1;
				int max_moose_level = int(floor(0.5f + sqrtf(2*max_moose_count - 0.25f)));

				int moose_count = 0;
				for (int z = 0; z < max_moose_level; ++z)
				{
					int inv_z = max_moose_level - z - 1;
//					int max_this_level = powf(2, inv_z);
					int max_this_level = 1 + inv_z;
					int start_this_level = 0;
					if (z == 0) start_this_level = (max_moose_level * (max_moose_level+1))/2 - max_moose_count;

					float fiz = 1.0f + (float(inv_z)) / 2;
					for (int j = start_this_level; j < max_this_level; ++j)
					{
						float lj = j - (float(max_this_level - 1) / 2);

						float x = lj + (float(pos) / 4);
						float y = - 0.15f + 0.5f;

						x /= fiz;
						y /= fiz;
						float lw = w / fiz;
						float lh = h / fiz;

						blit(
							device,
							moose_anim.getFrame(float(frame) / 3),
							tex_fx,
							x - lw / 2,
							y - lh / 2 - 0.5f,
							lw,
							lh
						);
					}
				}
			}

			float blomst_01_boost = 1 + (1.0f / (1.0f + fmod((beat - 0) / 3, 1.0f))) * 0.25f;
			float blomst_02_boost = 1 + (1.0f / (1.0f + fmod((beat - 1) / 3, 1.0f))) * 0.25f;
			float blomst_03_boost = 1 + (1.0f / (1.0f + fmod((beat - 2) / 3, 1.0f))) * 0.25f;

			blomst_01_img.w = 0.25f * blomst_01_boost;
			blomst_01_img.h = 0.73f * blomst_01_boost;
			blomst_01_img.x = -1 + 0.22f - (blomst_01_img.w / 2);
			blomst_01_img.y = -1;
			blomst_01_img.draw(device);

			blomst_02_img.w = 0.25f * blomst_02_boost;
			blomst_02_img.h = 0.6f * blomst_02_boost;
			blomst_02_img.x = -1 + 0.55f - blomst_02_img.w / 2;
			blomst_02_img.y = -1;
			blomst_02_img.draw(device);

			blomst_03_img.w = 0.5f * blomst_03_boost;
			blomst_03_img.h = 0.8f * blomst_03_boost;
			blomst_03_img.x = 0.13f - blomst_03_img.w / 2;
			blomst_03_img.y = -1;
			blomst_03_img.draw(device);

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			device.setRenderTarget(rt2.getRenderTarget());
/*			device.setDepthStencilSurface(Surface(NULL)); */
/*			device->SetRenderState(D3DRS_ZENABLE,  FALSE); */
			device->SetDepthStencilSurface(NULL);
			color_msaa.resolve(device);

			device->Clear(0, 0, D3DCLEAR_TARGET, clear_color, 1.f, 0);

			Matrix4x4 texcoord_transform;
			Matrix4x4 texture_transform = texture_matrix(color_msaa);

			Vector3 texcoord_translate(
				0.5 + (0.5 / color_msaa.getSurface().getDesc().Width),
				0.5 + (0.5 / color_msaa.getSurface().getDesc().Height),
				0.0);
			blur_fx->SetFloatArray("texcoord_translate", texcoord_translate, 2);
			texcoord_transform.make_scaling(Vector3(1,1,1));
			blur_fx->SetMatrix("texcoord_transform", &texcoord_transform);
			blur_fx->SetMatrix("texture_transform", &texture_transform);

			float blur_amt = 0.05f;
			float amt = 1.0f / (1 + blur_amt * 0.02f);
			Vector2 blur_center(sin(time) * cos(time * 0.3), cos(time * 0.99) * sin(time * 0.4));
			Matrix4x4 texel_transform = radialblur_matrix(color_msaa, blur_center, amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);
#if 0

			blit(device, rt, blur_fx, -1, -1, 2, 2);
//			blit(device, blurme1_tex, blur_fx, polygon);

			amt = 1.0 / (1 + blur_amt * 0.04f);
			texel_transform = radialblur_matrix(rt, blur_center, amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);

			core::d3dErr(device->SetRenderTarget(0, rt3));
			rt2.resolve();

			device->Clear(0, 0, D3DCLEAR_TARGET, clear_color, 1.f, 0);
			blit(device, rt2, blur_fx, -1, -1, 2, 2);

			amt = 1.0 / (1 + blur_amt * 0.08f);
			texel_transform = radialblur_matrix(rt, blur_center, amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);

			core::d3dErr(device->SetRenderTarget(0, rt2));
			rt3.resolve();
			device->Clear(0, 0, D3DCLEAR_TARGET, clear_color, 1.f, 0);
			blit(device, rt3, blur_fx, -1, -1, 2, 2);

#endif
			amt = 1.0f / (1 + blur_amt * 0.16f);
			texel_transform = radialblur_matrix(color_msaa, blur_center, amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);

			core::d3dErr(device->SetRenderTarget(0, backbuffer));
			color_msaa.resolve(device);
			device->Clear(0, 0, D3DCLEAR_TARGET, clear_color, 1.f, 0);
			
//			tex_transform.make_scaling(Vector3(config.getWidth() / 32, config.getHeight() / 32, 1.0f));
			tex_transform.make_scaling(Vector3(32.0f / config.getWidth(), 32.0f / config.getHeight(), 1.0f));
			pixelize_fx->SetMatrix("tex_transform", &tex_transform);
			blit(device, color_msaa, pixelize_fx, -1, -1, 2, 2);

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			device->EndScene(); /* WE DONE IS! */
			
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

				/* handle keys-events */
				if (WM_QUIT == msg.message) done = true;
				if (WM_KEYDOWN == msg.message)
				{
					if (VK_ESCAPE == LOWORD(msg.wParam)) done = true;
					if (VK_SPACE == LOWORD(msg.wParam)) printf("beat: %f - time %f\n", beat, time);

					if (VK_LEFT == LOWORD(msg.wParam))
					{
						ms = LEFT;
						pos -= 1;
						if (pos < -3) pos = -3;
						move_time = beat;
					}

					if (VK_RIGHT == LOWORD(msg.wParam))
					{
						ms = RIGHT;
						pos += 1;
						if (pos > 3) pos = 3;
						move_time = beat;
					}
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
