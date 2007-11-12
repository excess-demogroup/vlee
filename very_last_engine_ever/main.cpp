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

#include "engine/voxelgrid.h"
//#define GRID_SIZE (128)
#define GRID_SIZE (96)
//#define GRID_SIZE (64)
//#define GRID_SIZE (32)
//#define GRID_SIZE (16)

#define VOXEL_DATA_SIZE (32)
engine::VoxelGrid<VOXEL_DATA_SIZE> voxelgrid;

void fill_grid(BYTE grid[GRID_SIZE][GRID_SIZE][GRID_SIZE], float fgrid_size, const math::Vector3 &c1, const math::Vector3 &c2)
{
	int grid_size = int(floor(fgrid_size));
	for (int z = 0; z < grid_size; ++z)
	{
		for (int y = 0; y < grid_size; ++y)
		{
			for (int x = 0; x < grid_size; ++x)
			{
#if 0
				Vector3 p(float(x) / (grid_size / 2), float(y) / (grid_size / 2), float(z) / (grid_size / 2));
				Vector3 p2 = mrot * p;

				float x2 = p2.x * (GRID_SIZE / 2) + (GRID_SIZE / 2);
				float y2 = p2.y * (GRID_SIZE / 2) + (GRID_SIZE / 2);
				float z2 = p2.z * (GRID_SIZE / 2) + (GRID_SIZE / 2);

				int ix = int(floor(x2));
				int iy = int(floor(y2));
				int iz = int(floor(z2));

				if (ix <= 0 || ix >= GRID_SIZE-1) continue;
				if (iy <= 0 || iy >= GRID_SIZE-1) continue;
				if (iz <= 0 || iz >= GRID_SIZE-1) continue;

				float dist = grid.trilinearSample(x2, y2, z2);

				float size = (0.5f / grid_size) - dist * 0.5f;
				grid[z][y][x] = BYTE(math::clamp(size, 0.0f, 1.0f) * 255);
#else
				float fx = float(x) - c1.x;
				float fy = float(y) - c1.y;
				float fz = float(z) - c1.z;
				float fx2 = float(x) - c2.x;
				float fy2 = float(y) - c2.y;
				float fz2 = float(z) - c2.z;

/*
				float fx = float(x) * (M_PI / GRID_SIZE);
				float fy = float(y) * (M_PI / GRID_SIZE);
				float fz = float(z) * (M_PI / GRID_SIZE);
*/
				float size = 1.0f / (fx * fx + fy * fy + fz * fz);
				size += 1.0f / (fx2 * fx2 + fy2 * fy2 + fz2 * fz2);

				size -= (0.75f / fgrid_size);
//				size *= 8;
				size *= fgrid_size / 0.75f;

//				size *= fgrid_size;
//				float size = 1.0 / sqrt(fx * fx + fy * fy + fz * fz);
//				size = size * 3;
//				size /= 8 * 8 * 8 * 2;
//				size *= GRID_SIZE / 64;
//				size = 0.5;
//				if (size > 1.75) size = 0;

				grid[z][y][x] = BYTE(math::clamp(size, 0.0f, 1.0f) * 255);
//				grid[z][y][x] = BYTE(math::clamp(size, 0.0f, 1.0f) * 255);
#endif
			}
		}
	}
}

void fill_grid2(BYTE grid[GRID_SIZE][GRID_SIZE][GRID_SIZE], Matrix4x4 mrot, float grid_size)
{
	int igrid_min_size = int(floor(grid_size) / 2);
	int igrid_max_size = int(ceil(grid_size) / 2);

#if 0
	igrid_min_size /= 4;
	igrid_max_size /= 4;
#endif

	float translate = VOXEL_DATA_SIZE / 2;
	float scale = VOXEL_DATA_SIZE / 2;

	Matrix4x4 mtranslate, mscale;
	mtranslate.make_translation(Vector3(translate, translate, translate));
	mscale.make_scaling(Vector3(scale, scale, scale));
	mrot *= mscale * mtranslate;

	float grid_size_rcp = 1.0f / (grid_size / 2);

	Vector3 dx = Vector3(mrot._11, mrot._12, mrot._13) * grid_size_rcp;
	Vector3 dy = Vector3(mrot._21, mrot._22, mrot._23) * grid_size_rcp;
	Vector3 dz = Vector3(mrot._31, mrot._32, mrot._33) * grid_size_rcp;

	Vector3 pz(
		float(-igrid_min_size) * grid_size_rcp,
		float(-igrid_min_size) * grid_size_rcp,
		float(-igrid_min_size) * grid_size_rcp
	);
	pz = math::mul(mrot, pz);

	int dx_x = int(dx.x * (1 << 24));
	int dx_y = int(dx.y * (1 << 24));
	int dx_z = int(dx.z * (1 << 24));

	const int high_threshold = 1;
	const int low_threshold = -1;

	for (int z = -igrid_min_size; z < igrid_max_size; ++z)
	{
		Vector3 py = pz;
		pz += dz;

		for (int y = -igrid_min_size; y < igrid_max_size; ++y)
		{
			int px_x = int(py.x * (1 << 24));
			int px_y = int(py.y * (1 << 24));
			int px_z = int(py.z * (1 << 24));

			py += dy;

			for (int x = -igrid_min_size; x < igrid_max_size; ++x)
			{
				int px = px_x;
				int py = px_y;
				int pz = px_z;

				px_x += dx_x;
				px_y += dx_y;
				px_z += dx_z;

				int ix = px >> 24;
				int iy = py >> 24;
				int iz = pz >> 24;

				if (
					ix <= 0 || ix >= VOXEL_DATA_SIZE-1 ||
					iy <= 0 || iy >= VOXEL_DATA_SIZE-1 ||
					iz <= 0 || iz >= VOXEL_DATA_SIZE-1)
				{
					grid[z + igrid_min_size][y + igrid_min_size][x + igrid_min_size] = 0;
					continue;
				}

				int index = voxelgrid.getIndex(ix, iy, iz);

				if (voxelgrid.min_distances[index] > high_threshold)
				{
					grid[z + igrid_min_size][y + igrid_min_size][x + igrid_min_size] = 0;
					continue;
				}
				if (voxelgrid.max_distances[index] < low_threshold)
				{
					grid[z + igrid_min_size][y + igrid_min_size][x + igrid_min_size] = 255;
					continue;
				}

#if 0
				float dist = voxelgrid.trilinearSample(
					float(px) / (1 << 24),
					float(py) / (1 << 24),
					float(pz) / (1 << 24)
				) / 128;
#else
				float dist = float(voxelgrid.trilinearSample(px, py, pz)) / (128);
//				float dist = voxelgrid.pointSample(ix, iy, iz) / 128.0f;
#endif
				dist *= sqrtf(VOXEL_DATA_SIZE * VOXEL_DATA_SIZE * VOXEL_DATA_SIZE) / 2;
//				dist *= 64;

				float size = (0.5f / grid_size) - dist * 0.5f;
				grid[z + igrid_min_size][y + igrid_min_size][x + igrid_min_size] = BYTE(math::clamp(size, 0.0f, 1.0f) * 255);
			}
		}
	}
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
		
		RenderTexture color_msaa(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8, config.getMultisample());
		Surface depthstencil_msaa = device.createDepthStencilSurface(letterbox_viewport.Width, letterbox_viewport.Height, D3DFMT_D24S8, config.getMultisample());

		/** DEMO ***/

//		RenderTexture rt(device, 128, 128, 1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE);
		RenderTexture rt2(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8);
		RenderTexture rt3(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8);

		Matrix4x4 tex_transform;
		tex_transform.make_identity();
		Effect tex_fx      = engine::loadEffect(device, "data/tex.fx");
		tex_fx->SetMatrix("transform", &tex_transform);
		Effect blur_fx     = engine::loadEffect(device, "data/blur.fx");

		Image color_image(color_msaa, tex_fx);

		renderer::CubeTexture cube = engine::loadCubeTexture(device, "data/stpeters_cross2.dds");

		Effect test_fx = engine::loadEffect(device, "data/test.fx");
		test_fx->SetTexture("env", cube);

		Mesh cube_x = engine::loadMesh(device, "data/cube.X");
		Effect cubegrid_fx = engine::loadEffect(device, "data/cubegrid.fx");

		renderer::VolumeTexture front_tex = engine::loadVolumeTexture(device, "data/front.dds");
		cubegrid_fx->SetTexture("front_tex", front_tex);


		const D3DVERTEXELEMENT9 vertex_elements[] =
		{
			/* static data */
			{ 0, 0, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, // pos
			{ 0, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 }, // normal + front index
			/* instance data */
			{ 1, 0, D3DDECLTYPE_UBYTE4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // pos2
			{ 1, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 }, // instance array
			{ 1, 8, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 }, // instance array
			D3DDECL_END()
		};
		renderer::VertexDeclaration vertex_decl = device.createVertexDeclaration(vertex_elements);

		renderer::VertexBuffer static_vb  = device.createVertexBuffer(6 * 4 * (4 * 2), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED);
		{
			BYTE *dst = (BYTE*)static_vb.lock(0, 6 * 4 * (4 * 2), 0);
			
			/* front face (positive z) */
			*dst++ = 0;   *dst++ = 0;   *dst++ = 255; *dst++ = 255;
			*dst++ = 0;   *dst++ = 0;   *dst++ = 0;   *dst++ = 0; // <0,0>, 0, 0

			*dst++ = 255; *dst++ = 0;   *dst++ = 255; *dst++ = 255;
			*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 0; // <0,0,1>, 0

			*dst++ = 0;   *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 0; // <0,0,1>, 0

			*dst++ = 255; *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 0; // <0,0,1>, 0

			/* back face (negative z)*/
			*dst++ = 255; *dst++ = 0;   *dst++ = 0; *dst++ = 255;
			*dst++ = 0;   *dst++ = 0;   *dst++ = 0; *dst++ = 1; // <0,0,-1>, 5

			*dst++ = 0;   *dst++ = 0;   *dst++ = 0; *dst++ = 255;
			*dst++ = 255; *dst++ = 0;   *dst++ = 0; *dst++ = 1; // <0,0,-1>, 5

			*dst++ = 255; *dst++ = 255; *dst++ = 0; *dst++ = 255;
			*dst++ = 0;   *dst++ = 255; *dst++ = 0; *dst++ = 1; // <0,0,-1>, 5

			*dst++ = 0;   *dst++ = 255; *dst++ = 0; *dst++ = 255;
			*dst++ = 255; *dst++ = 255; *dst++ = 0; *dst++ = 1; // <0,0,-1>, 5

			/* top face (positive y)*/
			*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 255;
			*dst++ = 0;   *dst++ = 0;   *dst++ = 0; *dst++ = 2; // <0,1,0>, 5

			*dst++ = 0;   *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 2; // <0,1,0>, 5

			*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 255;
			*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 2; // <0,1,0>, 5

			*dst++ = 255; *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 2; // <0,1,0>, 5

			/* bottom face (negative y) */
			*dst++ = 0;   *dst++ = 0;   *dst++ = 255; *dst++ = 255;
			*dst++ = 0;   *dst++ = 0;   *dst++ = 0;   *dst++ = 3; // <0,-1,0>, 5

			*dst++ = 0;   *dst++ = 0;   *dst++ = 0;   *dst++ = 255;
			*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 3; // <0,-1,0>, 5

			*dst++ = 255; *dst++ = 0;   *dst++ = 255; *dst++ = 255;
			*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 3; // <0,-1,0>, 5

			*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 255;
			*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 3; // <0,-1,0>, 5

			/* left face (positive x)*/
			*dst++ = 255; *dst++ = 0;   *dst++ = 255; *dst++ = 255;
			*dst++ = 0;   *dst++ = 0;   *dst++ = 127; *dst++ = 4; // <1,0,0>, 5

			*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 255;
			*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 4; // <1,0,0>, 5

			*dst++ = 255; *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 4; // <1,0,0>, 5

			*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 255;
			*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 4; // <1,0,0>, 5

			/* right face (negative x)*/
			*dst++ = 0;   *dst++ = 0;   *dst++ = 0;   *dst++ = 255;
			*dst++ = 0;   *dst++ = 0;   *dst++ = 0;   *dst++ = 5; // <-1,0,0>, 5

			*dst++ = 0;   *dst++ = 0;   *dst++ = 255; *dst++ = 255;
			*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 5; // <-1,0,0>, 5

			*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 255;
			*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 5; // <-1,0,0>, 5

			*dst++ = 0;   *dst++ = 255; *dst++ = 255; *dst++ = 255;
			*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 5; // <-1,0,0>, 5

			static_vb.unlock();
		}
		
		int vb_switcher = 0;
		renderer::VertexBuffer dynamic_vbs[2];
		dynamic_vbs[0] = device.createVertexBuffer(GRID_SIZE * GRID_SIZE * GRID_SIZE * (4 * 3), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT);
		dynamic_vbs[1] = device.createVertexBuffer(GRID_SIZE * GRID_SIZE * GRID_SIZE * (4 * 3), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT);

/*		dynamic_vbs[0] = device.createVertexBuffer(GRID_SIZE * GRID_SIZE * GRID_SIZE * (4 * 3), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_SYSTEMMEM);
		dynamic_vbs[1] = device.createVertexBuffer(GRID_SIZE * GRID_SIZE * GRID_SIZE * (4 * 3), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_SYSTEMMEM); */
		
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

		FILE *fp = fopen("data/duck.voxel", "rb");
		if (NULL == fp) throw FatalException("failed to load voxel");
		float max_dist;
		fread(&max_dist, 4, 1, fp);
		for (int z = 0; z < VOXEL_DATA_SIZE; ++z)
		{
			for (int y = 0; y < VOXEL_DATA_SIZE; ++y)
			{
				for (int x = 0; x < VOXEL_DATA_SIZE; ++x)
				{
					signed char bdist;
					fread(&bdist, 1, 1, fp);
					voxelgrid.setDistance(x, y, z, bdist);
				}
			}
		}
		fclose(fp);


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
//			time = timeGetTime() / 1000.0f;

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
				float(sin(time * 0.125f * 4)),
				float(cos(time * 0.125f * 5)),
				float(cos(time * 0.135f * 3))
			);
			eye = normalize(eye);

//			float grid_size = 1.0f;
//			float grid_size = ((2 + float(cos(time))) / 4) * GRID_SIZE;
			float grid_size = (1 - cos(time / 4)) / 2;
//			grid_size = pow(grid_size, 0.5f);
			grid_size *= GRID_SIZE;

			eye *= grid_size;
			Vector3 at(floor(grid_size / 2), floor(grid_size / 2), floor(grid_size / 2));

			D3DXMATRIX world;
			D3DXMatrixIdentity(&world);
			D3DXMATRIX view;
			D3DXMatrixLookAtLH(&view, &(eye + at), &at, &up);
			D3DXMATRIX proj;
			D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(60), DEMO_ASPECT, 0.1f, 1000.f);

			test_fx.setMatrices(world, view, proj);
			test_fx->SetFloat("fade", 1.0f);
			test_fx->SetFloat("mask_fade", 1.0f);

			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

//			test_fx.draw(cube_x);

			cubegrid_fx.setMatrices(world, view, proj);

//			time /= 4;
			static BYTE grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
#if 0
			float cx = (0.5f + sin(time) / 3) * grid_size;
			float cy = (0.5f + cos(time) / 3) * grid_size;
			float cz = (0.5f + sin(time) / 3) * grid_size;
			math::Vector3 c1(cx, cy, cz);
			float cx2 = (0.5f + float(cos(time - M_PI) / 3) * grid_size;
			float cy2 = (0.5f + sin(time - M_PI) / 3) * grid_size;
			float cz2 = (0.5f + sin(time - M_PI) / 3) * grid_size;
			math::Vector3 c2(cx2, cy2, cz2);
			fill_grid(grid, fgrid_size, c1, c2);
#else
			Matrix4x4 mrot;
			mrot.make_identity();
			mrot.make_rotation(Vector3(0, time, 0));
			mrot.make_rotation(Vector3(float(-M_PI / 2), float(M_PI - sin(time / 5)), float(M_PI + time / 3)));
			Matrix4x4 mscale;
			float scale = 0.75f;
			mscale.make_scaling(Vector3(scale, scale, scale));
			fill_grid2(grid, mrot * mscale, grid_size);
#endif

			int cubes = 0;
			int culled = 0;
			{

				int igrid_min_size = int(floor(grid_size) / 2);
				int igrid_max_size = int(ceil(grid_size) / 2);
				int igrid_size = igrid_max_size + igrid_min_size;

				BYTE *dst = (BYTE*)dynamic_vbs[vb_switcher & 1].lock(0, igrid_size * igrid_size * igrid_size * (4 * 3), 0);
				for (int z = 0; z < igrid_size; ++z)
				{
					for (int y = 0; y < igrid_size; ++y)
					{
						for (int x = 0; x < igrid_size; ++x)
						{
							BYTE size = grid[z][y][x];

							if (size == 0) continue;
							if (
								(x > 0 && x < igrid_size - 1) &&
								(y > 0 && y < igrid_size - 1) &&
								(z > 0 && z < igrid_size - 1)
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

							/* neighbour info: 6 centers, 12 edges, 8 corners */
							/* x = center, y = even edge, z = odd edge, w = */
							/* tc0 - x y z w */
							/* tc1 - x y z w */
							/* tc2 - x y z w */
							/* tc3 - x y z w */
							/* tc4 - x y z w */
							/* tc5 - x y z w */
							/* tc6 - x y z w */

							/* fill in centre faces */
							*dst++ = z < igrid_size - 1 ? grid[z+1][y][x] : 0; // +z
							*dst++ = z > 0 ?              grid[z-1][y][x] : 0; // -z

							*dst++ = y < igrid_size - 1 ? grid[z][y+1][x] : 0; // +y
							*dst++ = y > 0 ?              grid[z][y-1][x] : 0; // -y

							*dst++ = x < igrid_size - 1 ? grid[z][y][x+1] : 0; // +x
							*dst++ = x > 0 ?              grid[z][y][x-1] : 0; // -x

#if 0
							/* fill in edge faces */
							// front layer (z+1)
							*dst++ = (y < igrid_size - 1 && z < igrid_size - 1) ? grid[z+1][y+1][x] : 0; // +y +z
							*dst++ = (y > 0              && z < igrid_size - 1) ? grid[z+1][y-1][x] : 0; // -y +z
							*dst++ = (x < igrid_size - 1 && z < igrid_size - 1) ? grid[z+1][y][x+1] : 0; // +x +z
							*dst++ = (x > 0              && z < igrid_size - 1) ? grid[z+1][y][x-1] : 0; // -x +z

							// middle layer (z)
							*dst++ = (x > 0              && y > 0             ) ? grid[z][y-1][x-1] : 0; // -x -y
							*dst++ = (x < igrid_size - 1 && y > 0             ) ? grid[z][y-1][x+1] : 0; // +x -y
							*dst++ = (x > 0              && y < igrid_size - 1) ? grid[z][y+1][x-1] : 0; // -x +y
							*dst++ = (x < igrid_size - 1 && y < igrid_size - 1) ? grid[z][y+1][x+1] : 0; // +x +y

							// bottom layer (z-1)
							*dst++ = (y < igrid_size - 1 && z > 0             ) ? grid[z-1][y+1][x] : 0; // +y -z
							*dst++ = (y > 0              && z > 0             ) ? grid[z-1][y-1][x] : 0; // -y -z
							*dst++ = (x < igrid_size - 1 && z > 0             ) ? grid[z-1][y][x+1] : 0; // +x -z
							*dst++ = (x > 0              && z > 0             ) ? grid[z-1][y][x-1] : 0; // -x -z
#endif

							/* fill in corners (?) */

							*dst++ = 128;
							*dst++ = 128;

							cubes++;
						}
					}
				}
				dynamic_vbs[vb_switcher & 1].unlock();
			}
//			printf("cubes: %d culled: %d\n", cubes, culled);

			/* setup render state */
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

			/* setup vs input */
			device->SetVertexDeclaration(vertex_decl);
			device->SetStreamSource(0, static_vb, 0, 4 * 2);
			device->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | cubes);
			device->SetStreamSource(1, dynamic_vbs[vb_switcher & 1], 0, 4 * 3);
			device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1UL);
			vb_switcher++;


			device->SetIndices(ib);

			/* draw */
			UINT passes;
			cubegrid_fx->Begin(&passes, 0);
			for (UINT pass = 0; pass < passes; ++pass)
			{
				cubegrid_fx->BeginPass( pass );
				device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 6 * 4, 0, 6 * 2);
				cubegrid_fx->EndPass();
			}
			cubegrid_fx->End();

			/* back to normal */
			device->SetStreamSourceFreq(0, 1);
			device->SetStreamSourceFreq(1, 1);

			/* letterbox */
			device.setRenderTarget(backbuffer);
			device->SetDepthStencilSurface(NULL);
			device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.f, 0);
			device.setViewport(&letterbox_viewport);

			color_msaa.resolve(device);

			color_image.setPosition(-1, -1);
			color_image.setDimension(2, 2);
			color_image.draw(device);
			
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
