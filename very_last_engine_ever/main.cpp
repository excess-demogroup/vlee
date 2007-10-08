#include "stdafx.h"
#include "resource.h"
#include "config.h"
#include "configdialog.h"
#include "init.h"

#include "core/fatalexception.h"

#include "math/vector2.h"
#include "math/vector3.h"
#include "math/matrix4x4.h"
#include "math/math.h"
#include "renderer/device.h"
#include "renderer/surface.h"
#include "renderer/texture.h"
#include "renderer/rendertexture.h"
#include "renderer/vertexbuffer.h"
#include "renderer/indexbuffer.h"
#include "renderer/vertexdeclaration.h"
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

void makeLetterboxViewport(D3DVIEWPORT9 *viewport, int screen_width, int screen_height, float screen_aspect, float demo_aspect)
{
	int letterbox_width, letterbox_height;

	if (demo_aspect > screen_aspect)
	{
		/* demo is wider than screen, letterbox */
		float aspect_change = screen_aspect / demo_aspect;
		letterbox_width  = screen_width;
		letterbox_height = int(math::round(screen_height * aspect_change));
	}
	else
	{
		/* screen is wider than demo, pillarbox */
		float aspect_change = demo_aspect / screen_aspect;
		letterbox_width  = int(math::round(screen_width * aspect_change));
		letterbox_height = screen_height;
	}

	viewport->X = (screen_width  - letterbox_width ) / 2;
	viewport->Y = (screen_height - letterbox_height) / 2;

	viewport->Width  = letterbox_width;
	viewport->Height = letterbox_height;
}

int main(int /*argc*/, char* /*argv*/ [])
{
#ifndef NDEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
/*	_CrtSetBreakAlloc(68); */
#endif
	
	_Module.Init(NULL, GetModuleHandle(0));
	
	try
	{
		/* create d3d object */
		CComPtr<IDirect3D9> direct3d = com_ptr(Direct3DCreate9(D3D_SDK_VERSION));
		if (!direct3d) throw FatalException("your directx-version is from the stone-age.\n\nTHRUG SAYS: UPGRADE!");
		
		ConfigDialog config(direct3d);
		
#ifdef NDEBUG
		/* show config dialog */
		INT_PTR result = config.DoModal();
		if (FAILED(result)) MessageBox(NULL, "Could not initialize dialogbox, using default settings.", NULL, MB_OK);
		else
		{
			if (IDOK != result)
			{
				// cancel was hit...
				MessageBox(NULL, "damn wimp...", "pfff", MB_OK);
				return 0;
			}
		}
#endif
		
		/* create window */
		win = CreateWindow("static", "very last engine ever", WS_POPUP, 0, 0, config.getWidth(), config.getHeight(), 0, 0, GetModuleHandle(0), 0);
		if (!win) throw FatalException("CreateWindow() failed. something is VERY spooky.");
		
		/* create device */
		Device device;
		device.Attach(init::initD3D(direct3d, win, config.getMode(), D3DMULTISAMPLE_NONE, config.getAdapter(), config.getVsync()));
		
		/* showing window after initing d3d in order to be able to see warnings during init */
		ShowWindow(win, TRUE);
#if !WINDOWED
		ShowCursor(0);
#endif
		
		/* setup letterbox */
		D3DVIEWPORT9 letterbox_viewport = device.getViewport();
		makeLetterboxViewport(&letterbox_viewport, config.getWidth(), config.getHeight(), config.getAspect(), DEMO_ASPECT);
		
		/* setup sound-playback */
		if (!BASS_Init(config.getSoundcard(), 44100, BASS_DEVICE_LATENCY, 0, 0)) throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/208_skritt_til_venstre.mp3", 0, 0, BASS_MP3_SETPOS | ((0 == config.getSoundcard()) ? BASS_STREAM_DECODE : 0));
		if (!stream) throw FatalException("failed to open tune");
		
		SyncTimerBASS_Stream synctimer(stream, BPM, 4);
		
#ifdef SYNC
		SyncEditor sync("data\\__data_%s_%s.sync", synctimer);
#else
		Sync sync("data\\__data_%s_%s.sync", synctimer);
#endif
		
		Surface backbuffer   = device.getRenderTarget(0);
		Surface depthstencil = device.getDepthStencilSurface();
		
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

		CComPtr<IDirect3DCubeTexture9> cube;
		core::d3dErr(D3DXCreateCubeTextureFromFile(device, "data/stpeters_cross2.dds", &cube));

		Effect test_fx = engine::loadEffect(device, "data/test.fx");
		test_fx->SetTexture("env", cube);

		Mesh cube_x = engine::loadMesh(device, "data/cube.X");

		Effect cubegrid_fx = engine::loadEffect(device, "data/cubegrid.fx");

		const D3DVERTEXELEMENT9 vertex_elements[] =
		{
			/* static data */
			{ 0, 0, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, // pos
			{ 0, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 }, // normal + front index
			/* instance data */
			{ 1, 0, D3DDECLTYPE_UBYTE4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 }, // pos2
			{ 1, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // instance array
			{ 1, 8, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 }, // instance array
			D3DDECL_END()
		};
		renderer::VertexDeclaration vertex_decl = device.createVertexDeclaration(vertex_elements);

		renderer::VertexBuffer static_vb  = device.createVertexBuffer(6 * 4 * (4 * 2), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED);
		{
			BYTE *dst = (BYTE*)static_vb.lock(0, 6 * 4 * (4 * 2), 0);
			
			/* front face (positive z) */
			*dst++ = 0;   *dst++ = 0;   *dst++ = 255; *dst++ = 255;
			*dst++ = 127; *dst++ = 127; *dst++ = 255; *dst++ = 0; // <0,0,1>, 0

			*dst++ = 255; *dst++ = 0;   *dst++ = 255; *dst++ = 255;
			*dst++ = 127; *dst++ = 127; *dst++ = 255; *dst++ = 0; // <0,0,1>, 0

			*dst++ = 0;   *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 127; *dst++ = 127; *dst++ = 255; *dst++ = 0; // <0,0,1>, 0

			*dst++ = 255; *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 127; *dst++ = 127; *dst++ = 255; *dst++ = 0; // <0,0,1>, 0

			/* back face (negative z)*/
			*dst++ = 255; *dst++ = 0;   *dst++ = 0; *dst++ = 255;
			*dst++ = 127; *dst++ = 127; *dst++ = 0; *dst++ = 1; // <0,0,-1>, 5

			*dst++ = 0;   *dst++ = 0;   *dst++ = 0; *dst++ = 255;
			*dst++ = 127; *dst++ = 127; *dst++ = 0; *dst++ = 1; // <0,0,-1>, 5

			*dst++ = 255; *dst++ = 255; *dst++ = 0; *dst++ = 255;
			*dst++ = 127; *dst++ = 127; *dst++ = 0; *dst++ = 1; // <0,0,-1>, 5

			*dst++ = 0;   *dst++ = 255; *dst++ = 0; *dst++ = 255;
			*dst++ = 127; *dst++ = 127; *dst++ = 0; *dst++ = 1; // <0,0,-1>, 5

			/* top face (positive y)*/
			*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 255;
			*dst++ = 127; *dst++ = 255; *dst++ = 127; *dst++ = 2; // <0,1,0>, 5

			*dst++ = 0;   *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 127; *dst++ = 255; *dst++ = 127; *dst++ = 2; // <0,1,0>, 5

			*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 255;
			*dst++ = 127; *dst++ = 255; *dst++ = 127; *dst++ = 2; // <0,1,0>, 5

			*dst++ = 255; *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 127; *dst++ = 255; *dst++ = 127; *dst++ = 2; // <0,1,0>, 5

			/* bottom face (negative y) */
			*dst++ = 0;   *dst++ = 0; *dst++ = 255; *dst++ = 255;
			*dst++ = 127; *dst++ = 0; *dst++ = 127; *dst++ = 3; // <0,-1,0>, 5

			*dst++ = 0;   *dst++ = 0; *dst++ = 0;   *dst++ = 255;
			*dst++ = 127; *dst++ = 0; *dst++ = 127; *dst++ = 3; // <0,-1,0>, 5

			*dst++ = 255; *dst++ = 0; *dst++ = 255; *dst++ = 255;
			*dst++ = 127; *dst++ = 0; *dst++ = 127; *dst++ = 3; // <0,-1,0>, 5

			*dst++ = 255; *dst++ = 0; *dst++ = 0;   *dst++ = 255;
			*dst++ = 127; *dst++ = 0; *dst++ = 127; *dst++ = 3; // <0,-1,0>, 5

			/* left face (positive x)*/
			*dst++ = 255; *dst++ = 0;   *dst++ = 255; *dst++ = 255;
			*dst++ = 255; *dst++ = 127; *dst++ = 127; *dst++ = 4; // <1,0,0>, 5

			*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 255;
			*dst++ = 255; *dst++ = 127; *dst++ = 127; *dst++ = 4; // <1,0,0>, 5

			*dst++ = 255; *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 255; *dst++ = 127; *dst++ = 127; *dst++ = 4; // <1,0,0>, 5

			*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 255;
			*dst++ = 255; *dst++ = 127; *dst++ = 127; *dst++ = 4; // <1,0,0>, 5

			/* right face (negative x)*/
			*dst++ = 0; *dst++ = 0;   *dst++ = 0;   *dst++ = 255;
			*dst++ = 0; *dst++ = 127; *dst++ = 127; *dst++ = 5; // <-1,0,0>, 5

			*dst++ = 0; *dst++ = 0;   *dst++ = 255; *dst++ = 255;
			*dst++ = 0; *dst++ = 127; *dst++ = 127; *dst++ = 5; // <-1,0,0>, 5

			*dst++ = 0; *dst++ = 255; *dst++ = 0;   *dst++ = 255;
			*dst++ = 0; *dst++ = 127; *dst++ = 127; *dst++ = 5; // <-1,0,0>, 5

			*dst++ = 0; *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 0; *dst++ = 127; *dst++ = 127; *dst++ = 5; // <-1,0,0>, 5

			static_vb.unlock();
		}

#define GRID_SIZE (64)

		renderer::VertexBuffer dynamic_vb = device.createVertexBuffer(GRID_SIZE * GRID_SIZE * GRID_SIZE * (4 * 3), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT);
		
		renderer::IndexBuffer ib = device.createIndexBuffer(2 * 6 * 6, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED);
		{
			unsigned short *dst = (unsigned short*)ib.lock(0, 2 * 6 * 6, 0);
			
			int faces = 6;
			for (int i = 0; i < faces; ++i)
			{
				*dst++ = (i * 4) + 0;
				*dst++ = (i * 4) + 1;
				*dst++ = (i * 4) + 2;
				*dst++ = (i * 4) + 3;
				*dst++ = (i * 4) + 2;
				*dst++ = (i * 4) + 1;
			}
			
			ib.unlock();
		}

			int cubes = 0;
			{
				BYTE *dst = (BYTE*)dynamic_vb.lock(0, GRID_SIZE * GRID_SIZE * GRID_SIZE * 4, 0);
				for (int z = 0; z < GRID_SIZE; ++z)
				{
					for (int y = 0; y < GRID_SIZE; ++y)
					{
						for (int x = 0; x < GRID_SIZE; ++x)
						{
							float size = 0.5;
							*dst++ = x; *dst++ = y; *dst++ = z;
//							*dst++ = 32; // (1 + cos(time - x - y)) * 127.5;
							*dst++ = BYTE(size * 255);
							cubes++;
						}
					}
				}
				dynamic_vb.unlock();
			}


		BASS_Start();
		BASS_ChannelPlay(stream, false);
		BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, 0.0f) + 10);


#ifdef SYNC
		sync.showEditor();
#endif

		timeBeginPeriod(1);
		DWORD lastFrameTime = timeGetTime();

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

#ifndef VJSYS
			sync.update(); //gets current timing info from the SyncTimer.
#endif
			device->BeginScene();
			
			/* setup multisampled stuffz */
			device.setRenderTarget(color_msaa.getRenderTarget());
			device.setDepthStencilSurface(depthstencil_msaa);
/*			
			D3DVIEWPORT9 viewport = device.getViewport();
			viewport.Width = 128;
			viewport.Height = 12;
			device.setViewport(&viewport);
*/
			time /= 2;
			D3DXCOLOR clear_color(1,0,0,0);
			device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, clear_color, 1.f, 0);

			Vector3 up(float(sin(time * 0.1f)), float(cos(time * 0.1f)), 0.f);
			Vector3 eye(
				float(sin(time * 0.25f)),
				float(cos(time * 0.25f)),
				float(cos(time * 0.35f))
			);
			eye = normalize(eye);
			eye *= GRID_SIZE;
			Vector3 at(GRID_SIZE / 2, GRID_SIZE / 2, GRID_SIZE / 2);

			D3DXMATRIX world;
			D3DXMatrixIdentity(&world);
			D3DXMATRIX view;
			D3DXMatrixLookAtLH(&view, &(eye + at), &at, &up);
			D3DXMATRIX proj;
			D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(70), DEMO_ASPECT, 0.1f, 100.f);

			test_fx.setMatrices(world, view, proj);
			test_fx->SetFloat("fade", 1.0f);
			test_fx->SetFloat("mask_fade", 1.0f);

			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

//			test_fx.draw(cube_x);

			cubegrid_fx.setMatrices(world, view, proj);

			time /= 4;
			float cx = 0.5f + sin(time) / 3;
			float cy = 0.5f + cos(time) / 3;
			float cz = 0.5f + sin(time / 3) / 3;

			float cx2 = 0.5f + cos(time * 0.9f) / 3;
			float cy2 = 0.5f + sin(time - 0.1f) / 3;
			float cz2 = 0.5f + sin(time / 2) / 3;

#if 1
			static BYTE grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
			int cubes = 0;
			int culled = 0;
			{
				for (int z = 0; z < GRID_SIZE; ++z)
				{
					for (int y = 0; y < GRID_SIZE; ++y)
					{
						for (int x = 0; x < GRID_SIZE; ++x)
						{
							float fx = float(x) * (1.0f / GRID_SIZE) - cx;
							float fy = float(y) * (1.0f / GRID_SIZE) - cy;
							float fz = float(z) * (1.0f / GRID_SIZE) - cz;
							float fx2 = float(x) * (1.0f / GRID_SIZE) - cx2;
							float fy2 = float(y) * (1.0f / GRID_SIZE) - cy2;
							float fz2 = float(z) * (1.0f / GRID_SIZE) - cz2;

/*
							float fx = float(x) * (M_PI / GRID_SIZE);
							float fy = float(y) * (M_PI / GRID_SIZE);
							float fz = float(z) * (M_PI / GRID_SIZE);
*/
							float size = 1.0f / (fx * fx + fy * fy + fz * fz);
							size += 1.0f / (fx2 * fx2 + fy2 * fy2 + fz2 * fz2);
							size -= 8.0f * 4;
//							float size = 1.0 / sqrt(fx * fx + fy * fy + fz * fz);
//							size = size * 3;
							size /= 8;
//							size *= GRID_SIZE / 64;
//							size = 0.5;
//							if (size > 1.75) size = 0;

							grid[z][y][x] = BYTE(math::clamp(size, 0.0f, 1.0f) * 255);
						}
					}
				}

				BYTE *dst = (BYTE*)dynamic_vb.lock(0, GRID_SIZE * GRID_SIZE * GRID_SIZE * (4 * 3), 0);
				for (int z = 0; z < GRID_SIZE; ++z)
				{
					for (int y = 0; y < GRID_SIZE; ++y)
					{
						for (int x = 0; x < GRID_SIZE; ++x)
						{
							BYTE size = grid[z][y][x];

							if (size == 0) continue;
							if (
								(x > 0 && x < GRID_SIZE - 1) &&
								(y > 0 && y < GRID_SIZE - 1) &&
								(z > 0 && z < GRID_SIZE - 1)
								)
							{

								if (
									grid[z][y][x-1] == 255 &&
									grid[z][y][x+1] == 255 &&
									grid[z][y-1][x] == 255 &&
									grid[z][y+1][x] == 255 &&
									grid[z-1][y][x] == 255 &&
									grid[z+1][y][x] == 255
									)
								{
									culled++;
									continue;
								}
							}

							*dst++ = x; *dst++ = y; *dst++ = z;
							*dst++ = size;

							*dst++ = z < GRID_SIZE - 1 ? grid[z+1][y][x] : 0; // +z
							*dst++ = z > 0 ?             grid[z-1][y][x] : 0; // -z

							*dst++ = y < GRID_SIZE - 1 ? grid[z][y+1][x] : 0; // +y
							*dst++ = y > 0 ?             grid[z][y-1][x] : 0; // -y

							*dst++ = x < GRID_SIZE - 1 ? grid[z][y][x+1] : 0; // +x
							*dst++ = x > 0 ?             grid[z][y][x-1] : 0; // -x

							*dst++ = 128;
							*dst++ = 128;

							cubes++;
						}
					}
				}
				dynamic_vb.unlock();
			}
			printf("cubes: %d culled: %d\n", cubes, culled);
#endif
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			device->SetVertexDeclaration(vertex_decl);

			device->SetStreamSource(0, static_vb, 0, 4 * 2);
			device->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | cubes);

			device->SetStreamSource(1, dynamic_vb, 0, 4 * 3);
			device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1UL);

			device->SetIndices(ib);

			UINT passes;
			cubegrid_fx->Begin(&passes, 0);
			for (UINT pass = 0; pass < passes; ++pass)
			{
				cubegrid_fx->BeginPass( pass );
				device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 6 * 4, 0, 6 * 2);
				cubegrid_fx->EndPass();
			}
			cubegrid_fx->End();

			device->SetStreamSourceFreq(0, 1);
			device->SetStreamSourceFreq(1, 1);

#if 0
			// Stream zero is our model, and its frequency is how we communicate the number of instances required,
			// which in this case is the total number of boxes
			V( pd3dDevice->SetStreamSource( 0, g_pVBBox, 0, sizeof(BOX_VERTEX)) );
			V( pd3dDevice->SetStreamSourceFreq( 0, D3DSTREAMSOURCE_INDEXEDDATA | g_NumBoxes ) );
			    
			// Stream one is the instancing buffer, so this advances to the next value
			// after each box instance has been drawn, so the divider is 1.
			V( pd3dDevice->SetStreamSource( 1, g_pVBInstanceData, 0, sizeof( BOX_INSTANCEDATA_POS ) ) );
			V( pd3dDevice->SetStreamSourceFreq( 1, D3DSTREAMSOURCE_INSTANCEDATA | 1ul ) );

			V( pd3dDevice->SetIndices( g_pIBBox ) );
#endif

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

//			core::d3dErr(device->SetRenderTarget(0, backbuffer));

			/* letterbox */
			device.setRenderTarget(backbuffer);
			device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.f, 0);
			device.setViewport(&letterbox_viewport);

			color_msaa.resolve(device);
//			tex_transform.make_scaling(Vector3(config.getWidth() / 32, config.getHeight() / 32, 1.0f));
//			tex_transform.make_scaling(Vector3(64.0f / config.getWidth(), 64.0f / config.getHeight(), 1.0f));
//			tex_transform.make_scaling(Vector3(64.0f / config.getWidth(), 64.0f / config.getHeight(), 1.0f));
//			pixelize_fx->SetMatrix("tex_transform", &tex_transform);
			blit(device, color_msaa, pixelize_fx, -1, -1, 2, 2);

			device->EndScene(); /* WE DONE IS! */
			
			HRESULT res = device->Present(0, 0, 0, 0);

			DWORD frameTime = timeGetTime();
			printf("frameTime: %d\n", frameTime - lastFrameTime);
			lastFrameTime = frameTime;
			
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
