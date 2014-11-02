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
#include "math/notrand.h"

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
#include "engine/particlestreamer.h"
#include "engine/particlecloud.h"

#include "engine/explosion.h"
#include "engine/ccbsplines.h"
#include "engine/grow.h"

#include "engine/voxelgrid.h"
#include "engine/voxelmesh.h"
#include "engine/cubestreamer.h"

#include "engine/textureproxy.h"
#include "engine/spectrumdata.h"
#include "engine/video.h"

#include <sync.h>

using math::Vector2;
using math::Vector3;
using math::Matrix4x4;

using renderer::Device;
using renderer::Surface;
using renderer::Texture;
using renderer::CubeTexture;
using renderer::VolumeTexture;
using renderer::RenderTexture;

using engine::Mesh;
using engine::Effect;
using engine::Anim;

using namespace core;

void makeLetterboxViewport(D3DVIEWPORT9 *viewport, int w, int h, float monitor_aspect, float demo_aspect)
{
	float backbuffer_aspect = float(w) / h;
	float w_ratio = 1.0f,
	      h_ratio = (monitor_aspect / demo_aspect) / (demo_aspect / backbuffer_aspect);

	if (h_ratio > 1.0f) {
		/* pillar box, yo! */
		w_ratio /= h_ratio;
		h_ratio = 1.0f;
	}

	viewport->Width = int(math::round(w * w_ratio));
	viewport->Height = int(math::round(h * h_ratio));
	viewport->X = (w - viewport->Width) / 2;
	viewport->Y = (h - viewport->Height) / 2;
}

const int rpb = 8; /* rows per beat */
const double row_rate = (double(BPM) / 60) * rpb;

double bass_get_row(HSTREAM h)
{
	QWORD pos = BASS_ChannelGetPosition(h, BASS_POS_BYTE);
	double time = BASS_ChannelBytes2Seconds(h, pos);
#ifndef SYNC_PLAYER
	return time * row_rate + 0.005;
#else
	return time * row_rate;
#endif
}

#ifndef SYNC_PLAYER

void bass_pause(void *d, int flag)
{
	if (flag)
		BASS_ChannelPause((HSTREAM)d);
	else
		BASS_ChannelPlay((HSTREAM)d, false);
}

void bass_set_row(void *d, int row)
{
	QWORD pos = BASS_ChannelSeconds2Bytes((HSTREAM)d, row / row_rate);
	BASS_ChannelSetPosition((HSTREAM)d, pos, BASS_POS_BYTE);
}

int bass_is_playing(void *d)
{
	return BASS_ChannelIsActive((HSTREAM)d) == BASS_ACTIVE_PLAYING;
}

struct sync_cb bass_cb = {
	bass_pause,
	bass_set_row,
	bass_is_playing
};

#endif /* !defined(SYNC_PLAYER) */

int main(int argc, char *argv[])
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
/*	_CrtSetBreakAlloc(68); */
#endif

	HINSTANCE hInstance = GetModuleHandle(0);
	HWND win = 0;
	HSTREAM stream = 0;

	try {
		if (!D3DXCheckVersion(D3D_SDK_VERSION, D3DX_SDK_VERSION)) {
			ShellExecute(NULL, "open", "http://www.gamesforwindows.com/directx/", NULL, NULL, SW_SHOWNORMAL);
			throw FatalException("Please download a newer version of the DirectX runtime from http://www.gamesforwindows.com/directx/");
		}

		/* create d3d object */
		ComRef<IDirect3D9> direct3d;
		direct3d.attachRef(Direct3DCreate9(D3D_SDK_VERSION));
		assert(direct3d);

		/* show config dialog */
		INT_PTR result = config::showDialog(hInstance, direct3d);
		if (FAILED(result))
			MessageBox(NULL, "Could not initialize dialogbox, using default settings.", NULL, MB_OK);
		else {
			if (IDOK != result) {
				// cancel was hit...
				MessageBox(NULL, "damn wimp...", "pfff", MB_OK);
				return 0;
			}
		}

		WNDCLASSEX wc;
		wc.cbSize        = sizeof(WNDCLASSEX);
		wc.style         = 0;
		wc.lpfnWndProc   = DefWindowProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = hInstance;
		wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)NULL;
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = "d3dwin";
		wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
		if (!RegisterClassEx(&wc))
			throw FatalException("RegisterClassEx() failed.");

		DWORD ws = config::fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
		RECT rect = {0, 0, config::mode.Width, config::mode.Height};
		AdjustWindowRect(&rect, ws, FALSE);
		win = CreateWindow("d3dwin", "very last engine ever", ws, 0, 0, rect.right - rect.left, rect.bottom - rect.top, 0, 0, hInstance, 0);
		if (!win)
			throw FatalException("CreateWindow() failed.");

		GetClientRect(win, &rect);
		config::mode.Width = rect.right - rect.left;
		config::mode.Height = rect.bottom - rect.top;

		/* create device */
		Device device;
		device.attachRef(init::initD3D(direct3d, win, config::mode, D3DMULTISAMPLE_NONE, config::adapter, config::vsync, config::fullscreen));

		/* showing window after initing d3d in order to be able to see warnings during init */
		ShowWindow(win, TRUE);
		if (config::fullscreen)
			ShowCursor(FALSE);

		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

//		device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.f, 0);
		HRESULT res = device->Present(0, 0, 0, 0);
		if (FAILED(res))
			throw FatalException(std::string(DXGetErrorString(res)) + std::string(" : ") + std::string(DXGetErrorDescription(res)));

		/* setup letterbox */
		D3DVIEWPORT9 letterbox_viewport = device.getViewport();
		makeLetterboxViewport(&letterbox_viewport, config::mode.Width, config::mode.Height, config::aspect, float(DEMO_ASPECT));

		/* setup sound-playback */
		if (!BASS_Init(config::soundcard, 44100, 0, 0, 0))
			throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/kick_me3.mp3", 0, 0, BASS_MP3_SETPOS | BASS_STREAM_PRESCAN | ((0 == config::soundcard) ? BASS_STREAM_DECODE : 0));
		if (!stream)
			throw FatalException("failed to open tune");

		sync_device *rocket = sync_create_device("data/sync");
		if (!rocket)
			throw FatalException("something went wrong - failed to connect to host?");

#ifndef SYNC_PLAYER
		if (sync_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
			throw FatalException("failed to connect to host");
#endif

		/** DEMO ***/

		const sync_track *partTrack = sync_get_track(rocket, "part");

		const sync_track *cameraDistanceTrack   = sync_get_track(rocket, "cam.dist");
		const sync_track *cameraTimeTrack       = sync_get_track(rocket, "cam.time");
		const sync_track *cameraXTrack          = sync_get_track(rocket, "cam.x");
		const sync_track *cameraYTrack          = sync_get_track(rocket, "cam.y");
		const sync_track *cameraZTrack          = sync_get_track(rocket, "cam.z");
		const sync_track *cameraAtXTrack        = sync_get_track(rocket, "cam.at.x");
		const sync_track *cameraAtYTrack        = sync_get_track(rocket, "cam.at.y");
		const sync_track *cameraAtZTrack        = sync_get_track(rocket, "cam.at.z");
		const sync_track *cameraRollTrack       = sync_get_track(rocket, "cam.roll");
		const sync_track *cameraOffsetTrack     = sync_get_track(rocket, "cam.offset");
		const sync_track *cameraIndexTrack      = sync_get_track(rocket, "cam.index");
		const sync_track *cameraShakeAmtTrack   = sync_get_track(rocket, "cam.shake.amt");
		const sync_track *cameraShakeSpeedTrack = sync_get_track(rocket, "cam.shake.speed");

		const sync_track *colorMapFadeTrack    = sync_get_track(rocket, "cm.fade");
		const sync_track *colorMapFlashTrack   = sync_get_track(rocket, "cm.flash");
		const sync_track *colorMapOverlayTrack = sync_get_track(rocket, "cm.overlay");
		const sync_track *colorMapOverlayAlphaTrack = sync_get_track(rocket, "cm.overlay_alpha");
		const sync_track *pulseAmt2Track       = sync_get_track(rocket, "cm.pulse.amt");
		const sync_track *pulseSpeed2Track     = sync_get_track(rocket, "cm.pulse.speed");

		const sync_track *bloomCutoffTrack = sync_get_track(rocket, "bloom.cutoff");
		const sync_track *bloomShapeTrack  = sync_get_track(rocket, "bloom.shape");
		const sync_track *bloomAmtTrack    = sync_get_track(rocket, "bloom.amt");

		const sync_track *distAmtTrack     = sync_get_track(rocket, "dist.amt");
		const sync_track *distFreqTrack    = sync_get_track(rocket, "dist.freq");
		const sync_track *distOffsetTrack  = sync_get_track(rocket, "dist.offset");

		const sync_track *glitchBlockThreshTrack = sync_get_track(rocket, "glitch.blocks");
		const sync_track *glitchLineThreshTrack = sync_get_track(rocket, "glitch.lines");

		const sync_track *flareAmountTrack = sync_get_track(rocket, "flare.amount");

		const sync_track *skyboxDesaturateTrack = sync_get_track(rocket, "skybox.desat");

		const sync_track *clusterL1Idx = sync_get_track(rocket, "cluster.l1.idx");
		const sync_track *clusterL1Amt = sync_get_track(rocket, "cluster.l1.amt");
		const sync_track *clusterL2Idx = sync_get_track(rocket, "cluster.l2.idx");
		const sync_track *clusterL2Amt = sync_get_track(rocket, "cluster.l2.amt");

		const sync_track *dofFStopTrack = sync_get_track(rocket, "dof.fstop");
		const sync_track *dofFocalLengthTrack = sync_get_track(rocket, "dof.flen");
		const sync_track *dofFocalDistTrack = sync_get_track(rocket, "dof.fdist");

		Surface backbuffer   = device.getRenderTarget(0);

		D3DCAPS9 caps;
		direct3d->GetDeviceCaps(config::adapter, D3DDEVTYPE_HAL, &caps);

		bool use_sm20_codepath = false;
		if (FAILED(direct3d->CheckDeviceFormat(config::adapter, D3DDEVTYPE_HAL, config::mode.Format, D3DUSAGE_QUERY_FILTER | D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)) ||
			caps.PixelShaderVersion < D3DVS_VERSION(3, 0))
			use_sm20_codepath = true;

		RenderTexture color_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);
		RenderTexture depth_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_R32F);
		Surface depthstencil = device.createDepthStencilSurface(letterbox_viewport.Width, letterbox_viewport.Height, D3DFMT_D24S8);

		RenderTexture dof_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);
		RenderTexture dof_temp1_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);
		RenderTexture dof_temp2_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);

		RenderTexture fxaa_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);

#define MAP_SIZE 32
		renderer::Texture temp_tex = device.createTexture(MAP_SIZE * MAP_SIZE, MAP_SIZE, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED);
		std::vector<renderer::VolumeTexture> color_maps;
		for (int i = 0; true; ++i) {
			char temp[256];
			sprintf(temp, "data/color_maps/%04d.png", i);
			D3DXIMAGE_INFO info;
			if (FAILED(D3DXLoadSurfaceFromFile(temp_tex.getSurface(), NULL, NULL, temp, NULL, D3DX_FILTER_NONE, 0, &info)))
				break;

			D3DSURFACE_DESC desc = temp_tex.getSurface().getDesc();
			assert(desc.Format == D3DFMT_X8R8G8B8);

			if (info.Width != MAP_SIZE * MAP_SIZE || info.Height != MAP_SIZE)
				throw core::FatalException("color-map is of wrong size!");

			renderer::VolumeTexture cube_tex = device.createVolumeTexture(MAP_SIZE, MAP_SIZE, MAP_SIZE, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED);

			D3DLOCKED_RECT rect;
			core::d3dErr(temp_tex.getSurface()->LockRect(&rect, NULL, 0));
			D3DLOCKED_BOX box;
			core::d3dErr(cube_tex->LockBox(0, &box, NULL, 0));
			for (int z = 0; z < MAP_SIZE; ++z)
				for (int y = 0; y < MAP_SIZE; ++y)
					for (int x = 0; x < MAP_SIZE; ++x) {
						((unsigned char*)box.pBits)[z * 4 + y * box.RowPitch + x * box.SlicePitch + 0] = ((unsigned char*)rect.pBits)[(x + z * MAP_SIZE) * 4 + y * rect.Pitch + 0];
						((unsigned char*)box.pBits)[z * 4 + y * box.RowPitch + x * box.SlicePitch + 1] = ((unsigned char*)rect.pBits)[(x + z * MAP_SIZE) * 4 + y * rect.Pitch + 1];
						((unsigned char*)box.pBits)[z * 4 + y * box.RowPitch + x * box.SlicePitch + 2] = ((unsigned char*)rect.pBits)[(x + z * MAP_SIZE) * 4 + y * rect.Pitch + 2];
						((unsigned char*)box.pBits)[z * 4 + y * box.RowPitch + x * box.SlicePitch + 3] = ((unsigned char*)rect.pBits)[(x + z * MAP_SIZE) * 4 + y * rect.Pitch + 3];
					}
			cube_tex->UnlockBox(0);
			temp_tex.getSurface()->UnlockRect();
			color_maps.push_back(cube_tex);
		}
		if (0 == color_maps.size())
			throw core::FatalException("no color maps!");

		RenderTexture color1_hdr = RenderTexture(device, letterbox_viewport.Width, letterbox_viewport.Height, 0, D3DFMT_A16B16G16R16F);
		RenderTexture color2_hdr = RenderTexture(device, letterbox_viewport.Width, letterbox_viewport.Height, 0, D3DFMT_A16B16G16R16F);

		Effect *dof_fx = engine::loadEffect(device, "data/dof.fx");
		dof_fx->setVector3("viewport", Vector3(letterbox_viewport.Width, letterbox_viewport.Height, 0.0f));

		Effect *blur_fx      = engine::loadEffect(device, "data/blur.fx");

		Effect *fxaa_fx = engine::loadEffect(device, "data/fxaa.fx");
		fxaa_fx->setVector3("viewportInv", Vector3(1.0f / letterbox_viewport.Width, 1.0f / letterbox_viewport.Height, 0.0f));

		Effect *postprocess_fx = engine::loadEffect(device, "data/postprocess.fx");
		postprocess_fx->setVector3("viewport", Vector3(letterbox_viewport.Width, letterbox_viewport.Height, 0.0f));
		Texture lensdirt_tex = engine::loadTexture(device, "data/lensdirt.png");
		postprocess_fx->setTexture("lensdirt_tex", lensdirt_tex);

		Texture noise_tex = engine::loadTexture(device, "data/noise.png");
		postprocess_fx->setTexture("noise_tex", noise_tex);
		postprocess_fx->setVector3("nscale", Vector3(letterbox_viewport.Width / 256.0f, letterbox_viewport.Height / 256.0f, 0.0f));

		Texture spectrum_tex = engine::loadTexture(device, "data/spectrum.png");
		postprocess_fx->setTexture("spectrum_tex", spectrum_tex);

		engine::ParticleStreamer particleStreamer(device);
		Effect *particle_fx = engine::loadEffect(device, "data/particle.fx");
		Texture particle_tex = engine::loadTexture(device, "data/particle.png");
		particle_fx->setTexture("tex", particle_tex);

		Effect *cubes_fx = engine::loadEffect(device, "data/cubes.fx");
		engine::MeshInstancer cube_instancer(device, cubes_fx, 4096);

		Mesh *tunnel_x = engine::loadMesh(device, "data/tunnel.x");
		Effect *tunnel_fx = engine::loadEffect(device, "data/tunnel.fx");
		VolumeTexture volume_noise_tex = engine::loadVolumeTexture(device, "data/volume-noise.dds");
		tunnel_fx->setTexture("volume_noise_tex", volume_noise_tex);

		Mesh *skybox_x = engine::loadMesh(device, "data/skybox.x");
		Effect *skybox_fx = engine::loadEffect(device, "data/skybox.fx");
		CubeTexture skybox_tex = engine::loadCubeTexture(device, "data/skybox.dds");
		CubeTexture skybox2_tex = engine::loadCubeTexture(device, "data/skybox2.dds");

		Mesh *plane_128x128_x = engine::loadMesh(device, "data/plane-128x128.x");
		Effect *sphere_lights_fx = engine::loadEffect(device, "data/sphere-lights.fx");
		Anim sphere_lights = engine::loadAnim(device, "data/sphere-lights");
		Texture sphere_lights_mask_tex = engine::loadTexture(device, "data/sphere-lights-mask.png");
		sphere_lights_fx->setTexture("mask_tex", sphere_lights_mask_tex);
		sphere_lights_fx->setTexture("noise_tex", noise_tex);
		sphere_lights_fx->setVector2("nscale", Vector2(128.0f / noise_tex.getWidth(), 128.0f / noise_tex.getHeight()));

		Mesh *knot_x = engine::loadMesh(device, "data/knot.x");
		int numVertices = knot_x->getVertexCount();
		Vector3 *vertices = new Vector3[numVertices];
		knot_x->getVertexPositions(vertices, 0, numVertices);

		Anim overlays = engine::loadAnim(device, "data/overlays");

		bool dump_video = false;
		for (int i = 1; i < argc; ++i)
			if (!strcmp(argv[i], "--dump-video"))
				dump_video = true;

		if (dump_video)
			_mkdir("dump");

		BASS_Start();
		BASS_ChannelPlay(stream, false);

		bool done = false;
		int frame = 0;
		while (!done) {
			if (dump_video) {
				QWORD pos = BASS_ChannelSeconds2Bytes(stream, float(frame) / config::mode.RefreshRate);
				BASS_ChannelSetPosition(stream, pos, BASS_POS_BYTE);
			}

			double row = bass_get_row(stream);

#ifndef SYNC_PLAYER
			sync_update(rocket, int(row), &bass_cb, (void *)stream);
#endif
			double beat = row / 4;

			float camTime = sync_get_val(cameraTimeTrack, row);
			float camOffset = sync_get_val(cameraOffsetTrack, row);
			Vector3 camPos, camTarget, camUp = Vector3(0, 1, 0);
			switch ((int)sync_get_val(cameraIndexTrack, row)) {
			case 0:
				camTarget = Vector3(sync_get_val(cameraAtXTrack, row), sync_get_val(cameraAtYTrack, row), sync_get_val(cameraAtZTrack, row));
				camPos = camTarget + Vector3(sin(camTime / 2) * sync_get_val(cameraDistanceTrack, row),
					sync_get_val(cameraYTrack, row),
					cos(camTime / 2) * sync_get_val(cameraDistanceTrack, row));
				break;

			case 1:
				camPos = Vector3(sin(camTime * float(M_PI / 180)), cos(camTime * float(M_PI / 180)), 0) * sync_get_val(cameraDistanceTrack, row);
				camTarget = Vector3(sin((camTime + camOffset) * float(M_PI / 180)), cos((camTime + camOffset) * float(M_PI / 180)), 0) * sync_get_val(cameraDistanceTrack, row);
				camUp = camPos - camTarget;
				camUp = Vector3(camUp.y, camUp.z, camUp.x);
				break;

			case 2: {
				float angle = sync_get_val(cameraTimeTrack, row) * float(M_PI / 180);
				float angle2 = angle + sync_get_val(cameraOffsetTrack, row) * float(M_PI / 180);
				camPos = Vector3(sin(angle) * 30, 0, cos(angle) * 30);
				camPos += normalize(camPos) * sync_get_val(cameraYTrack, row);
				camTarget = Vector3(sin(angle2) * 30, 0, cos(angle2) * 30);
				camTarget += normalize(camTarget) * sync_get_val(cameraYTrack, row);
				} break;

			case 3:
				camPos = Vector3(sync_get_val(cameraXTrack, row), sync_get_val(cameraYTrack, row), sync_get_val(cameraZTrack, row));
				camTarget = Vector3(sync_get_val(cameraAtXTrack, row), sync_get_val(cameraAtYTrack, row), sync_get_val(cameraAtZTrack, row));
				break;

			default:
				camPos = Vector3(0, 1, 0) * sync_get_val(cameraDistanceTrack, row);
				camTarget = Vector3(0, 0, 0);
			}

			bool particles = false;
			bool tunnel = false;
			bool sphereLights = false;
			bool skybox = false;
			bool blackCubes = false;
			bool blueCubes = false;
			bool dof = true;
			bool space = false;
			bool particleObject = false;
			int dustParticleCount = 0;
			float dustParticleAlpha = 1.0f;

			int part = int(sync_get_val(partTrack, row));
			switch (part) {
			case 0:
				skybox = true;
				sphereLights = true;
				dustParticleCount = 30000;
				dustParticleAlpha = 0.1f;
				break;

			case 1:
				skybox = true;
				blackCubes = true;
				blueCubes = true;
				dustParticleCount = 10000;
				dustParticleAlpha = 0.3f;
				break;

			case 2:
				tunnel = true;
				break;

			case 3:
				space = true;
				dof = true; // false; <- does not work, wtf?
				break;

			case 4:
				skybox = true;
				particleObject = true;
				break;
			}

#ifdef SYNC_PLAYER
			if (part < 0)
				done = true;
#endif

			double shake_phase = beat * 32 * sync_get_val(cameraShakeSpeedTrack, row);
			Vector3 camOffs(sin(shake_phase), cos(shake_phase * 0.9), sin(shake_phase - 0.5));
			camPos += camOffs * sync_get_val(cameraShakeAmtTrack, row);
			camTarget += camOffs * sync_get_val(cameraShakeAmtTrack, row);

			float camRoll = sync_get_val(cameraRollTrack, row) * float(M_PI / 180);
			Matrix4x4 view;
			D3DXMatrixLookAtLH(&view, &camPos, &camTarget, &camUp);
			view *= Matrix4x4::rotation(Vector3(0, 0, camRoll));


			Matrix4x4 world = Matrix4x4::identity();
			Matrix4x4 proj  = Matrix4x4::projection(80.0f, float(DEMO_ASPECT), 1.0f, 10000.f);

			// render
			device->BeginScene();
			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			device.setRenderTarget(color_target.getRenderTarget(), 0);
			device.setRenderTarget(depth_target.getRenderTarget(), 1);
			device.setDepthStencilSurface(depthstencil);
			device->SetRenderState(D3DRS_ZENABLE, true);

			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			device->SetRenderState(D3DRS_ZWRITEENABLE, true);

			device->Clear(0, 0, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0xFF000000, 1.f, 0);

			if (skybox) {
				skybox_fx->setMatrices(world, view, proj);
				skybox_fx->setFloat("desaturate", sync_get_val(skyboxDesaturateTrack, row));
				skybox_fx->setTexture("env_tex", part != 0 ? skybox_tex : skybox2_tex);
				skybox_fx->commitChanges();
				skybox_fx->draw(skybox_x);
			}

			if (sphereLights) {
				sphere_lights_fx->setMatrices(world, view, proj);
				sphere_lights_fx->setTexture("intensity_tex", sphere_lights.getFrame(0));
				int scroll = int(row * 4.0) % 256 - 128;
				sphere_lights_fx->setFloat("scroll", scroll / 128.0f);
				sphere_lights_fx->setVector2("noffs", Vector2(
						floor(math::notRandf(int(beat * 100) + 0) * 128) / 128,
						floor(math::notRandf(int(beat * 100) + 1) * 128) / 128));
				sphere_lights_fx->commitChanges();
				sphere_lights_fx->draw(plane_128x128_x);
			}

			if (blackCubes) {
				cubes_fx->setMatrices(world, view, proj);
				cubes_fx->commitChanges();

				const int clusters = 8;
				int clusterBombs = 64;

				float clusterLightAmts[clusters] = { 0 };
				int l1 = int(sync_get_val(clusterL1Idx, row));
				int l2 = int(sync_get_val(clusterL2Idx, row));
				if (l1 >= 0 && l1 < ARRAY_SIZE(clusterLightAmts))
					clusterLightAmts[l1] = pow(sync_get_val(clusterL1Amt, row), 2.0f);
				if (l2 >= 0 && l2 < ARRAY_SIZE(clusterLightAmts))
					clusterLightAmts[l2] = pow(sync_get_val(clusterL2Amt, row), 2.0f);

				// bunch of stuff
				for (int i = 0; i < clusters; ++i) {
					Vector3 clusterPos(cos(i / 5.210f) * 30.0f, sin(i / 5.12f) * 40.0f, 0);
					Vector3 colors[] = {
						Vector3(1, 1, 2),
						Vector3(1.5, 1, 1.5)
					};
					Vector3 clusterColor = colors[i % ARRAY_SIZE(colors)] * clusterLightAmts[i];
					float clustert = math::clamp(float((beat / 16) - floor(beat / 16)), 0.0f, 1.0f);


					Matrix4x4 translation = Matrix4x4::translation(clusterPos);
					Matrix4x4 rotation = Matrix4x4::rotation(Vector3(sin(i * 1.0f) * 10.0f, sin(i * 1.412331f) * 10.0f, 0) * float(beat * 0.01f));
					Matrix4x4 scaling = Matrix4x4::scaling(Vector3(10, 10, 10));
					Matrix4x4 base = translation * rotation;

					cube_instancer.setInstanceTransform(i * clusterBombs, scaling * base);
					cube_instancer.setInstanceColor(i * clusterBombs, clusterColor);

					for (int j = 1; j < clusterBombs; ++j) {
						int idx = i * clusterBombs + j;
						float t = math::clamp(clustert * 2 - (j - 1), 0.0f, 1.0f);

						Vector3 pos = math::lerp(Vector3(sin(idx / 5.220f) * 80.0f, cos(idx / 5.10f) * 80.0f, 0), Vector3(0, 0, 0), t);
						Vector3 color = math::lerp(Vector3(0, 0, 0), clusterColor, pow(t, 20));

						Matrix4x4 translation = Matrix4x4::translation(pos);
						Matrix4x4 scaling = Matrix4x4::scaling(Vector3(1.5,1,1) * 2.0f * (1.5f + sin(idx / 5.120f)));
						Matrix4x4 rotation = Matrix4x4::rotation(Vector3(sin(idx * 1.0f) * 10.0f, sin(idx * 1.12311231f) * 10.0f, 0) * float(beat * 0.01f));

						cube_instancer.setInstanceTransform(idx, scaling * translation * rotation * base);
						cube_instancer.setInstanceColor(idx, color);
					}
				}
				cube_instancer.updateInstanceVertexBuffer();
				cube_instancer.draw(device, clusters * clusterBombs);
			}

			if (blueCubes) {
				cubes_fx->setMatrices(world, view, proj);
				cubes_fx->commitChanges();

				// flower-ish
				int num_cubes = 0;
				for (int i = 0; i < 8; ++i) {
					double th = i * ((2 * M_PI) / 8);
					Matrix4x4 curr = Matrix4x4::scaling(Vector3(2,1,4)) * Matrix4x4::rotation(Vector3(0, th, 0));
					for (int j = 0; j < 30; ++j) {
						Matrix4x4 rotation = Matrix4x4::rotation(Vector3(0, 0, 0.05));
						Matrix4x4 translation = Matrix4x4::translation(Vector3(1, 0, 0));
						Matrix4x4 scale = Matrix4x4::scaling(Vector3(1,1,1) * 0.9);
						curr = translation * rotation * scale * curr;
						cube_instancer.setInstanceTransform(num_cubes, curr);
						cube_instancer.setInstanceColor(num_cubes, math::Vector3(0.2,0.2,1) * pow(pow(float(cos(j / 10.0f - beat)), 2.0f), 10.0f) * 15.0);
						num_cubes++;
					}
				}
				cube_instancer.updateInstanceVertexBuffer();
				cube_instancer.draw(device, num_cubes);
			}

			if (tunnel) {
				Vector3 fogColor(0.3, 0.4, 0.6);
				tunnel_fx->setFloat("fogDensity", 0.0005);
				tunnel_fx->setVector3("fogColor", fogColor);
				tunnel_fx->setFloat("time", float(beat * 0.1));
				tunnel_fx->setMatrices(world, view, proj);
				tunnel_fx->commitChanges();
				tunnel_fx->draw(tunnel_x);

				cubes_fx->setMatrices(world, view, proj);
				cubes_fx->setFloat("fogDensity", 0.0005);
				cubes_fx->setVector3("fogColor", fogColor);
				cubes_fx->commitChanges();

				// bunch of stuff
				int a = 36, b = 16;
				for (int i = 0; i < a; ++i) {
					float th = i * float((2 * M_PI) / a);
					Matrix4x4 translation = Matrix4x4::translation(Vector3(65, 0, 0));
					Matrix4x4 rotation = Matrix4x4::rotation(Vector3(0, 0, th));
					Matrix4x4 base = translation * rotation;
					for (int j = 0; j < b; ++j) {
						float th = j * float((2 * M_PI) / b);
						Matrix4x4 translation = Matrix4x4::translation(Vector3(10, 0, 0));
						Matrix4x4 scale = Matrix4x4::scaling(Vector3(1, 10, 1));
						Matrix4x4 rotation = Matrix4x4::rotation(Vector3(0, th, 0));

						Matrix4x4 rotation2 = Matrix4x4::rotation(Vector3(cos(beat * 0.1) * 0.5, 0, 0));

						cube_instancer.setInstanceTransform(i * b + j, translation * scale * rotation2 * rotation * base);
						cube_instancer.setInstanceColor(i * b + j, math::Vector3(0, 0, 0));
					}
				}
				cube_instancer.updateInstanceVertexBuffer();
				cube_instancer.draw(device, a * b);

				cubes_fx->setFloat("fogDensity", 0.0);
				cubes_fx->setVector3("fogColor", Vector3(0, 0, 0));
			}

			if (dof) {
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
				device->SetRenderState(D3DRS_ZWRITEENABLE, false);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

				const float verts[] = {
					-0.5f,                                   -0.5f,                                    0.5f, 1.0f, 0.0f, 0.0f,
					-0.5f + float(letterbox_viewport.Width), -0.5f,                                    0.5f, 1.0f, 1.0f, 0.0f,
					-0.5f + float(letterbox_viewport.Width), -0.5f + float(letterbox_viewport.Height), 0.5f, 1.0f, 1.0f, 1.0f,
					-0.5f,                                   -0.5f + float(letterbox_viewport.Height), 0.5f, 1.0f, 0.0f, 1.0f,
				};
				device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

				dof_fx->p->Begin(NULL, 0);

				device.setRenderTarget(dof_target.getSurface(0), 0);
				device.setRenderTarget(NULL, 1);
				dof_fx->setTexture("color_tex", color_target);
				dof_fx->setTexture("depth_tex", depth_target);
				dof_fx->setFloat("focal_distance", sync_get_val(dofFocalDistTrack, row));
				dof_fx->setFloat("focal_length", sync_get_val(dofFocalLengthTrack, row));
				dof_fx->setFloat("f_stop", sync_get_val(dofFStopTrack, row));
				dof_fx->p->BeginPass(0);
				core::d3dErr(device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(float) * 6));
				dof_fx->p->EndPass();

				dof_fx->setTexture("premult_tex", dof_target);
				device.setRenderTarget(dof_temp1_target.getSurface(0), 0);
				device.setRenderTarget(dof_temp2_target.getSurface(0), 1);
				dof_fx->p->BeginPass(1);
				core::d3dErr(device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(float) * 6));
				dof_fx->p->EndPass();

				dof_fx->setTexture("temp1_tex", dof_temp1_target);
				dof_fx->setTexture("temp2_tex", dof_temp2_target);
				device.setRenderTarget(dof_target.getSurface(0), 0);
				device.setRenderTarget(NULL, 1);
				dof_fx->p->BeginPass(2);
				core::d3dErr(device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, verts, sizeof(float) * 6));
				dof_fx->p->EndPass();

				dof_fx->p->End();
			}

			if (tunnel) {
				device.setRenderTarget(dof_target.getSurface(0), 0);

				// particles
				Matrix4x4 modelview = world * view;
				Vector3 up(modelview._12, modelview._22, modelview._32);
				Vector3 left(modelview._11, modelview._21, modelview._31);
				math::normalize(up);
				math::normalize(left);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

				particle_fx->setVector3("up", up);
				particle_fx->setVector3("left", left);
				particle_fx->setMatrices(world, view, proj);
				particle_fx->setFloat("focal_distance", sync_get_val(dofFocalDistTrack, row));
				particle_fx->setFloat("focal_length", sync_get_val(dofFocalLengthTrack, row));
				particle_fx->setFloat("f_stop", sync_get_val(dofFStopTrack, row));
				particle_fx->setVector2("viewport", Vector2(letterbox_viewport.Width, letterbox_viewport.Height));

				particleStreamer.begin();
				for (int i = 0; i < 20 * 360; ++i) {
					float th = i * float((2 * M_PI) / 360);
					Vector3 pos = Vector3(sin(th), cos(th), 0) * 65;
					Vector3 offset = normalize(Vector3(
							sin(i * 32.0 + beat * 0.132),
							cos(i * 45.0 + beat * 0.21),
							cos(i * 23.0 - beat * 0.123)
							));
					pos += offset * 5;
					float size = 20.0f / (1 + math::notRandf(i) * 300.0f);
					particleStreamer.add(pos, size);
					if (!particleStreamer.getRoom()) {
						particleStreamer.end();
						particle_fx->draw(&particleStreamer);
						particleStreamer.begin();
					}
				}
				particleStreamer.end();
				particle_fx->draw(&particleStreamer);
			}

			if (dustParticleCount > 0) {
				device.setRenderTarget(dof_target.getSurface(0), 0);

				// particles
				Matrix4x4 modelview = world * view;
				Vector3 up(modelview._12, modelview._22, modelview._32);
				Vector3 left(modelview._11, modelview._21, modelview._31);
				math::normalize(up);
				math::normalize(left);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

				particle_fx->setVector3("up", up);
				particle_fx->setVector3("left", left);
				particle_fx->setMatrices(world, view, proj);
				particle_fx->setFloat("focal_distance", sync_get_val(dofFocalDistTrack, row));
				particle_fx->setFloat("focal_length", sync_get_val(dofFocalLengthTrack, row));
				particle_fx->setFloat("f_stop", sync_get_val(dofFStopTrack, row));
				particle_fx->setVector2("viewport", Vector2(letterbox_viewport.Width, letterbox_viewport.Height));

				particleStreamer.begin();
				for (int i = 0; i < dustParticleCount; ++i) {
					Vector3 pos = Vector3(math::notRandf(i) * 2 - 1, math::notRandf(i + 1) * 2 - 1, math::notRandf(i + 2) * 2 - 1) * 100;
					Vector3 offset = normalize(Vector3(
							sin(i * 0.23 + beat * 0.0532),
							cos(i * 0.27 + beat * 0.0521),
							cos(i * 0.31 - beat * 0.0512)
							));
					pos += offset * 10;
					double size = 5.0 / (3 + i * 0.001);
					particleStreamer.add(pos, float(size * dustParticleAlpha));
					if (!particleStreamer.getRoom()) {
						particleStreamer.end();
						particle_fx->draw(&particleStreamer);
						particleStreamer.begin();
					}
				}
				particleStreamer.end();
				particle_fx->draw(&particleStreamer);
			}

			if (particleObject) {
				device.setRenderTarget(dof_target.getSurface(0), 0);

				// particles
				Matrix4x4 modelview = world * view;
				Vector3 up(modelview._12, modelview._22, modelview._32);
				Vector3 left(modelview._11, modelview._21, modelview._31);
				math::normalize(up);
				math::normalize(left);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

				particle_fx->setVector3("up", up);
				particle_fx->setVector3("left", left);
				particle_fx->setMatrices(world, view, proj);
				particle_fx->setFloat("focal_distance", sync_get_val(dofFocalDistTrack, row));
				particle_fx->setFloat("focal_length", sync_get_val(dofFocalLengthTrack, row));
				particle_fx->setFloat("f_stop", sync_get_val(dofFStopTrack, row));
				particle_fx->setVector2("viewport", Vector2(letterbox_viewport.Width, letterbox_viewport.Height));

				particleStreamer.begin();
				for (int i = 0; i < numVertices; ++i) {
					Vector3 pos = vertices[i] * 30;
					double size = 1.0 / 10;
					particleStreamer.add(pos, float(size));
					if (!particleStreamer.getRoom()) {
						particleStreamer.end();
						particle_fx->draw(&particleStreamer);
						particleStreamer.begin();
					}
				}
				particleStreamer.end();
				particle_fx->draw(&particleStreamer);
			}

			if (space) {
				device.setRenderTarget(dof_target.getSurface(0), 0);

				// particles
				Matrix4x4 modelview = world * view;
				Vector3 up(modelview._12, modelview._22, modelview._32);
				Vector3 left(modelview._11, modelview._21, modelview._31);
				math::normalize(up);
				math::normalize(left);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

				particle_fx->setVector3("up", up);
				particle_fx->setVector3("left", left);
				particle_fx->setMatrices(world, view, proj);
				particle_fx->setFloat("focal_distance", sync_get_val(dofFocalDistTrack, row));
				particle_fx->setFloat("focal_length", sync_get_val(dofFocalLengthTrack, row));
				particle_fx->setFloat("f_stop", sync_get_val(dofFStopTrack, row));
				particle_fx->setVector2("viewport", Vector2(letterbox_viewport.Width, letterbox_viewport.Height));

				particleStreamer.begin();
				particleStreamer.add(Vector3(0, 0, 0), 50.0); // fake sun (make more better)
				for (int i = 0; i < 100000; ++i) {
					Vector3 pos = Vector3(math::notRandf(i) * 2 - 1, math::notRandf(i + 1) * 2 - 1, math::notRandf(i + 2) * 2 - 1) * 100;
					Vector3 offset = normalize(Vector3(
							sin(i * 0.23 + beat * 0.0532),
							cos(i * 0.27 + beat * 0.0521),
							cos(i * 0.31 - beat * 0.0512)
							));
					pos += offset * 10;

					double size = 10.0 / (1.5 + i * 0.0025);
					size *= 10.0 / (1.0 + math::length(pos) * 0.5);
					size *= std::max(0.0, sin(i * 0.22 + beat * 0.7532));

					particleStreamer.add(pos, float(size * dustParticleAlpha));
					if (!particleStreamer.getRoom()) {
						particleStreamer.end();
						particle_fx->draw(&particleStreamer);
						particleStreamer.begin();
					}
				}
				particleStreamer.end();
				particle_fx->draw(&particleStreamer);
			}

			device.setDepthStencilSurface(depthstencil);

			device.setRenderTarget(fxaa_target.getSurface(0), 0);
			device.setRenderTarget(color1_hdr.getSurface(), 1);
			fxaa_fx->setTexture("color_tex", dof ? dof_target : color_target);
			fxaa_fx->setFloat("bloom_cutoff", sync_get_val(bloomCutoffTrack, row));
			drawRect(device, fxaa_fx, 0, 0, float(letterbox_viewport.Width), float(letterbox_viewport.Height));
			device.setRenderTarget(NULL, 1);

			/* downsample and blur */
			float stdDev = 16.0f / 3;
			for (int i = 0; i < 7; ++i) {
				// copy to next level
				d3dErr(device->StretchRect(color1_hdr.getSurface(i), NULL, color1_hdr.getSurface(i + 1), NULL, D3DTEXF_LINEAR));

				/* do the bloom */
				device->SetDepthStencilSurface(NULL);
				device->SetRenderState(D3DRS_ZENABLE, false);

				for (int j = 0; j < 2; j++) {
					D3DXVECTOR4 gauss[8];
					float sigma_squared = stdDev * stdDev;
					double tmp = 1.0 / std::max(sqrt(2.0f * M_PI * sigma_squared), 1.0);
					float w1 = (float)tmp;
					w1 = std::max(float(w1 * 1.004 - 0.004), 0.0f);

					gauss[0].x = 0.0;
					gauss[0].y = 0.0;
					gauss[0].z = w1;
					gauss[0].w = 0.0;

					float total = w1;
					for (int k = 1; k < 8; ++k) {
						int o1 = k * 2 - 1;
						int o2 = k * 2;

						float w1 = float(tmp * exp(-o1 * o1 / (2.0f * sigma_squared)));
						float w2 = float(tmp * exp(-o2 * o2 / (2.0f * sigma_squared)));

						w1 = std::max(float(w1 * 1.004 - 0.004), 0.0f);
						w2 = std::max(float(w2 * 1.004 - 0.004), 0.0f);

						float w = w1 + w2;
						float o = (o1 * w1 + o2 * w2) / w;
						gauss[k].z = w;
						if (!j) {
							gauss[k].x = o / color1_hdr.getSurface(i).getWidth();
							gauss[k].y = 0.0f;
						} else {
							gauss[k].x = 0.0f;
							gauss[k].y = o / color1_hdr.getSurface(i).getHeight();
						}
						gauss[k].w = 0.0f;
						total += 2 * w;
					}

					// normalize weights
					for (int k = 0; k < 8; ++k)
						gauss[k].z /= total;

					blur_fx->p->SetVectorArray("gauss", gauss, 8);
					blur_fx->setFloat("lod", float(i));
					blur_fx->setTexture("blur_tex", j ? color2_hdr : color1_hdr);
					blur_fx->p->SetInt("size", 8);

					device.setRenderTarget(j ? color1_hdr.getSurface(i) : color2_hdr.getSurface(i), 0);
					int w = color1_hdr.getSurface(i).getWidth();
					int h = color1_hdr.getSurface(i).getHeight();
					drawRect(device, blur_fx, 0, 0, float(w), float(h));
				}
			}

			/* letterbox */
			device.setRenderTarget(backbuffer);
			device.setRenderTarget(NULL, 1);
			device->SetDepthStencilSurface(NULL);
			device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.f, 0);
			device.setViewport(&letterbox_viewport);

			float flash = sync_get_val(colorMapFlashTrack, row);
			float fade = sync_get_val(colorMapFadeTrack, row);
			float pulse = sync_get_val(pulseAmt2Track, row);
			fade = std::max(0.0f, fade - pulse + float(cos(beat * sync_get_val(pulseSpeed2Track, row) * M_PI)) * pulse);
			postprocess_fx->setVector3("noffs", Vector3(math::notRandf(int(beat * 100)), math::notRandf(int(beat * 100) + 1), 0));
			postprocess_fx->setFloat("flash", flash < 0 ? math::randf() : pow(flash, 2.0f));
			postprocess_fx->setFloat("fade", pow(fade, 2.2f));
			postprocess_fx->setFloat("dist_amt", sync_get_val(distAmtTrack, row) / 100);
			postprocess_fx->setFloat("dist_freq", sync_get_val(distFreqTrack, row) * 2 * float(M_PI));
			postprocess_fx->setFloat("dist_time", float(beat * 4) + sync_get_val(distOffsetTrack, row));
			postprocess_fx->setTexture("color_tex", fxaa_target);
			postprocess_fx->setFloat("overlay_alpha", sync_get_val(colorMapOverlayAlphaTrack, row));
			postprocess_fx->setTexture("overlay_tex", overlays.getTexture(int(sync_get_val(colorMapOverlayTrack, row)) % overlays.getTextureCount()));
			postprocess_fx->setTexture("bloom_tex", color1_hdr);
			postprocess_fx->setFloat("block_thresh", sync_get_val(glitchBlockThreshTrack, row));
			postprocess_fx->setFloat("line_thresh", sync_get_val(glitchLineThreshTrack, row));
			postprocess_fx->setFloat("flare_amount", sync_get_val(flareAmountTrack, row));
			float bloom_shape = sync_get_val(bloomShapeTrack, row);
			float bloom_weight[7];
			float bloom_total = 0;
			for (int i = 0; i < 7; ++i) {
				bloom_weight[i] = powf(float(i), bloom_shape);
				bloom_total += bloom_weight[i];
			}
			float bloom_scale = sync_get_val(bloomAmtTrack, row) / bloom_total;
			for (int i = 0; i < 7; ++i)
				bloom_weight[i] *= bloom_scale;
			postprocess_fx->setFloatArray("bloom_weight", bloom_weight, ARRAY_SIZE(bloom_weight));
			postprocess_fx->commitChanges();

			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			drawRect(device, postprocess_fx, float(letterbox_viewport.X), float(letterbox_viewport.Y), float(letterbox_viewport.Width), float(letterbox_viewport.Height));
			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			device->EndScene(); /* WE DONE IS! */

			if (dump_video) {
				char temp[256];
				_snprintf(temp, 256, "dump/frame%04d.tga", frame);
				core::d3dErr(D3DXSaveSurfaceToFile(
					temp,
					D3DXIFF_TGA,
					backbuffer,
					NULL,
					NULL
				));
			}

			HRESULT res = device->Present(0, 0, 0, 0);
			if (FAILED(res))
				throw FatalException(std::string(DXGetErrorString(res)) + std::string(" : ") + std::string(DXGetErrorDescription(res)));

			BASS_Update(0); // decrease the chance of missing vsync
			frame++;
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);

				/* handle keys-events */
				if (WM_QUIT == msg.message ||
				    WM_CLOSE == msg.message ||
				    (WM_KEYDOWN == msg.message && VK_ESCAPE == LOWORD(msg.wParam)))
					done = true;
			}
#ifdef SYNC_PLAYER
			if (BASS_ChannelIsActive(stream) == BASS_ACTIVE_STOPPED)
				done = true;
#endif
		}



		/** END OF DEMO ***/

		// cleanup
		sync_destroy_device(rocket);
		if (stream)
			BASS_StreamFree(stream);
		BASS_Free();
		if (win)
			DestroyWindow(win);
		if (config::fullscreen)
			ShowCursor(TRUE);
	} catch (const std::exception &e) {
		// cleanup
		if (stream)
			BASS_StreamFree(stream);
		BASS_Free();
		if (win)
			DestroyWindow(win);
		if (config::fullscreen)
			ShowCursor(TRUE);

		log::printf("\n*** error : %s\n", e.what());
		log::save("errorlog.txt");
		MessageBox(0, e.what(), 0, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
		return 1;
	}
	return 0;
}
