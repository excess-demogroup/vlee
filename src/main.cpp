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

Vector3 getCubeKeyframe(int key)
{
	const Vector3 keyframes[] = {
		Vector3(0,   0,   0),
		Vector3(60,  0,   0),
		Vector3(120, 0,   0),
		Vector3(120, 60,  0),
		Vector3(120, 120, 0),
		Vector3(60,  120, 0),
		Vector3(0,   120, 0),
		Vector3(0,   120, 60),
		Vector3(0,   120, 120),
		Vector3(0,   60,  120),
		Vector3(0,   0,   120),
		Vector3(0,   0,   60),
		Vector3(0,   0,   0),
		Vector3(0,   0,  -60),
		Vector3(0,   0,  -120),
		Vector3(60,  0,  -120),
		Vector3(120, 0,  -120),
		Vector3(120,-60,  -120),
		Vector3(120,-120,-120),
		Vector3(60, -120,-120),
		Vector3(0,  -120,-120),
		Vector3(0,  -120,-60),
		Vector3(0,  -120, 0),
		Vector3(0,  -60, 0)
	};
	if (key % 1)
		return keyframes[key % ARRAY_SIZE(keyframes)];
	else
		return keyframes[key % ARRAY_SIZE(keyframes)] * 0.5 + keyframes[(key - 1) % ARRAY_SIZE(keyframes)] * 0.25 + keyframes[(key + 1) % ARRAY_SIZE(keyframes)] * 0.25;
}

Vector3 getCubePos(float cam_time)
{
	int k0 = int(floor(cam_time) - 1);
	int k1 = k0 + 1;
	int k2 = k1 + 1;
	int k3 = k2 + 1;

	Vector3 samples[4] = {
		getCubeKeyframe(k0), getCubeKeyframe(k1), getCubeKeyframe(k2), getCubeKeyframe(k3)
	};

	Vector3 tangents[2] = {
		(samples[2] - samples[0]) / 2.0,
		(samples[3] - samples[1]) / 2.0
	};

	float t = cam_time - floor(cam_time);
	float t2 = t * t;
	float t3 = t2 * t;
	float w[4] = {
		2.0f * t3 - 3.0f * t2 + 1.0f,
		1.0f * t3 - 2.0f * t2 + t,
		w[2] = -2.0f * t3 + 3.0f * t2,
		w[3] =  1.0f * t3 - 1.0f * t2
	};

	return samples[1] * w[0] + tangents[0] * w[1] + samples[2] * w[2] + tangents[1] * w[3];
}

int main(int /*argc*/, char* /*argv*/ [])
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

		device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.f, 0);
		HRESULT res = device->Present(0, 0, 0, 0);
		if (FAILED(res))
			throw FatalException(std::string(DXGetErrorString(res)) + std::string(" : ") + std::string(DXGetErrorDescription(res)));

		/* setup letterbox */
		D3DVIEWPORT9 letterbox_viewport = device.getViewport();
		makeLetterboxViewport(&letterbox_viewport, config::mode.Width, config::mode.Height, config::aspect, float(DEMO_ASPECT));

		/* setup sound-playback */
		if (!BASS_Init(config::soundcard, 44100, 0, 0, 0))
			throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/tune.mp3", 0, 0, BASS_MP3_SETPOS | BASS_STREAM_PRESCAN | ((0 == config::soundcard) ? BASS_STREAM_DECODE : 0));
		if (!stream)
			throw FatalException("failed to open tune");

		sync_device *rocket = sync_create_device("data/sync");
		if (!rocket)
			throw FatalException("something went wrong - failed to connect to host?");

#ifndef SYNC_PLAYER
		if (sync_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
			throw FatalException("failed to connect to host");
#endif

		const sync_track *partTrack = sync_get_track(rocket, "part");

		const sync_track *cameraDistanceTrack   = sync_get_track(rocket, "cam.dist");
		const sync_track *cameraTimeTrack       = sync_get_track(rocket, "cam.time");
		const sync_track *cameraXTrack          = sync_get_track(rocket, "cam.x");
		const sync_track *cameraYTrack          = sync_get_track(rocket, "cam.y");
		const sync_track *cameraZTrack          = sync_get_track(rocket, "cam.z");
		const sync_track *cameraAtXTrack          = sync_get_track(rocket, "cam.at.x");
		const sync_track *cameraAtYTrack          = sync_get_track(rocket, "cam.at.y");
		const sync_track *cameraAtZTrack          = sync_get_track(rocket, "cam.at.z");
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

		const sync_track *distAmtTrack    = sync_get_track(rocket, "dist.amt");
		const sync_track *distFreqTrack   = sync_get_track(rocket, "dist.freq");
		const sync_track *distOffsetTrack = sync_get_track(rocket, "dist.offset");

		const sync_track *dofFStopTrack = sync_get_track(rocket, "dof.fstop");
		const sync_track *dofFocalLengthTrack = sync_get_track(rocket, "dof.flen");
		const sync_track *dofFocalDistTrack = sync_get_track(rocket, "dof.fdist");

		const sync_track *sphereSpeed1Track = sync_get_track(rocket, "sphere.speed1");
		const sync_track *sphereSpeed2Track = sync_get_track(rocket, "sphere.speed2");
		const sync_track *sphereFreq1Track = sync_get_track(rocket, "sphere.freq1");
		const sync_track *sphereFreq2Track = sync_get_track(rocket, "sphere.freq2");
		const sync_track *sphereAmt1Track = sync_get_track(rocket, "sphere.amt1");
		const sync_track *sphereAmt2Track = sync_get_track(rocket, "sphere.amt2");

		const sync_track *skyboxFadeTrack = sync_get_track(rocket, "skybox.fade");

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

		/** DEMO ***/

		Effect *dof_fx = engine::loadEffect(device, "data/dof.fx");
		dof_fx->setVector3("viewport", Vector3(letterbox_viewport.Width, letterbox_viewport.Height, 0.0f));

		Effect *fxaa_fx = engine::loadEffect(device, "data/fxaa.fx");
		fxaa_fx->setVector3("viewportInv", Vector3(1.0f / letterbox_viewport.Width, 1.0f / letterbox_viewport.Height, 0.0f));

		Effect *postprocess_fx = engine::loadEffect(device, "data/postprocess.fx");
		postprocess_fx->setVector3("viewport", Vector3(letterbox_viewport.Width, letterbox_viewport.Height, 0.0f));

		Texture noise_tex = engine::loadTexture(device, "data/noise.png");
		postprocess_fx->setTexture("noise_tex", noise_tex);
		postprocess_fx->setVector3("nscale", Vector3(letterbox_viewport.Width / 256.0f, letterbox_viewport.Height / 256.0f, 0.0f));

		Texture spectrum_tex = engine::loadTexture(device, "data/spectrum.png");
		postprocess_fx->setTexture("spectrum_tex", spectrum_tex);

		engine::ParticleStreamer particleStreamer(device);
		Effect *particle_fx = engine::loadEffect(device, "data/particle.fx");
		Texture particle_tex = engine::loadTexture(device, "data/particle.png");
		Texture darksmoke_tex = engine::loadTexture(device, "data/darksmoke.png");
		particle_fx->setTexture("tex", particle_tex);

		Mesh *sphere_x = engine::loadMesh(device, "data/sphere.x");
		Effect *sphere_fx = engine::loadEffect(device, "data/sphere.fx");
		CubeTexture bling_tex = engine::loadCubeTexture(device, "data/bling.dds");
		sphere_fx->setTexture("env_tex", bling_tex);
		Mesh *skybox_x = engine::loadMesh(device, "data/skybox.x");
		Effect *skybox_fx = engine::loadEffect(device, "data/skybox.fx");
		skybox_fx->setTexture("env_tex", bling_tex);

		Anim overlays = engine::loadAnim(device, "data/overlays");

		BASS_Start();
		BASS_ChannelPlay(stream, false);

		// todo: config this
		bool dump_video = false;

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
			bool use_roll = false;

			float camTime = float(beat / 4) + sync_get_val(cameraOffsetTrack, row);
			Vector3 camPos, camTarget, camUp;
			switch ((int)sync_get_val(cameraIndexTrack, row)) {
			case 0:
				camPos = Vector3(sin(camTime / 2) * sync_get_val(cameraDistanceTrack, row),
					sync_get_val(cameraYTrack, row),
					cos(camTime / 2) * sync_get_val(cameraDistanceTrack, row));
				camTarget = Vector3(0, 0, 0);
				camUp = Vector3(0, 1, 0);
				break;

			case 1:
				camPos = getCubePos(sync_get_val(cameraTimeTrack, row) / 16);
				camTarget = getCubePos(sync_get_val(cameraTimeTrack, row) / 16 + sync_get_val(cameraOffsetTrack, row));
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
				use_roll = true;
				} break;

			case 3:
				camPos = Vector3(sync_get_val(cameraXTrack, row), sync_get_val(cameraYTrack, row), sync_get_val(cameraZTrack, row));
				camTarget = Vector3(sync_get_val(cameraAtXTrack, row), sync_get_val(cameraAtYTrack, row), sync_get_val(cameraAtZTrack, row));
				use_roll = true;
				break;

			default:
				camPos = Vector3(0, 1, 0) * sync_get_val(cameraDistanceTrack, row);
				camTarget = Vector3(0, 0, 0);
				camUp = Vector3(0, 1, 0);
			}

			bool particles = false;
			bool light_particles = false;
			bool dark_particles = false;
			bool tunnel_particles = false;

			bool cluster = true;

			int part = int(sync_get_val(partTrack, row));
			switch (part) {
			case 0:
				cluster = true;
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
			if (use_roll)
				view = Matrix4x4::lookAt(camPos, camTarget, camRoll);
			else
				D3DXMatrixLookAtLH(&view, &camPos, &camTarget, &camUp);

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

//			float fog_density = sync_get_val(fogDensityTrack, row) / 100000;

//			Vector3 worldLightPosition = Vector3(0, sin(beat * 0.25) * 100, 0);
			float ltime = sync_get_val(cameraTimeTrack, row) / 16;
			Vector3 worldLightPosition = getCubePos(ltime);

			if (cluster) {
				// neuron cluster
				sphere_fx->setFloat("time1", float((beat / 8) * sync_get_val(sphereSpeed1Track, row)));
				sphere_fx->setFloat("time2", float((beat / 8) * sync_get_val(sphereSpeed2Track, row)));
				sphere_fx->setFloat("freq1", 1.0f / sync_get_val(sphereFreq1Track, row));
				sphere_fx->setFloat("freq2", 1.0f / sync_get_val(sphereFreq2Track, row));
				sphere_fx->setFloat("amt1", sync_get_val(sphereAmt1Track, row) / 100);
				sphere_fx->setFloat("amt2", sync_get_val(sphereAmt2Track, row) / 100);
				sphere_fx->setMatrices(world, view, proj);
				sphere_fx->commitChanges();
				sphere_fx->draw(sphere_x);

				skybox_fx->setFloat("fade", sync_get_val(skyboxFadeTrack, row));
				skybox_fx->setMatrices(world, view, proj);
				skybox_fx->commitChanges();
				skybox_fx->draw(skybox_x);
			}

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

			device.setRenderTarget(fxaa_target.getSurface(0), 0);
			device.setRenderTarget(NULL, 1);
			fxaa_fx->setTexture("color_tex", dof_target);
			drawRect(device, fxaa_fx, 0, 0, float(letterbox_viewport.Width), float(letterbox_viewport.Height));

			device.setDepthStencilSurface(depthstencil);
			if (particles) {
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
				particle_fx->setFloat("alpha", pow(1.0f, 2.2f));
				particle_fx->setMatrices(world, view, proj);

				if (dark_particles) {
					float dtime = sync_get_val(cameraTimeTrack, row) / 16;

					device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
					device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
					particle_fx->setTexture("tex", darksmoke_tex);
					particleStreamer.begin();
					const int num_particles = 500;
					for (int i = 2 * num_particles - 1; i != -1; --i) {
						int part = int(floor(dtime * num_particles - i));
						float ptime2 = float(part / float(num_particles));
						float ptime = float(beat * 0.01 + part * 135);
						Vector3 pos = getCubePos(part / float(num_particles));
						float prand = math::notRandf(part);
						Vector3 temp(cos(ptime + prand * 43) - sin(ptime * 0.8 + prand * 34), sin(ptime + prand * 12), sin(ptime + 20 + prand - 20));
						float woom = math::notRandf(part);
						pos += normalize(temp) * pow(woom, 1.5f) * 5;
						float fade = 1.0f;
						float dist = dtime - ptime2;
						float size = 5.0f / (1 + dist); // 0.3f / (1 + woom);
						if (dist < 0.1)
							size *= dist * 10;
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

				float light_alpha = 1;
				if (light_particles && light_alpha > 0.0f) {
					device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
					device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
					particle_fx->setTexture("tex", particle_tex);
					particle_fx->setFloat("alpha", pow(light_alpha, 2.2f));
					particleStreamer.begin();
					particleStreamer.add(worldLightPosition, 6.0f);
					for (int i = 0; i < 300; ++i) {
						float ptime = float(beat * 0.25 + i * 1235);
						float jalla = math::notRandf(i);
						Vector3 pos(cos(ptime + jalla * 24) - sin(ptime * 0.8 + jalla * 721), sin(ptime + jalla * 121), sin(ptime + 20 + jalla * 541));
						pos = normalize(pos) * 0.5;
	//					float woom = math::notRandf(i);
						float woom = float(0.5 + cos(math::notRandf(part) + ptime) * 0.5 + cos(beat + length(pos)) * 0.2);

						pos = worldLightPosition + normalize(pos) * (1 + woom) * 3;
						float size = 0.4f / (1 + woom);
		//				if (distance(pos, camPos) < 200)
						particleStreamer.add(pos, size);
						if (!particleStreamer.getRoom()) {
							particleStreamer.end();
							particle_fx->draw(&particleStreamer);
							particleStreamer.begin();
						}
					}

					const int num_particles = 200;
					for (int i = 0; i < num_particles; ++i) {
						int part = int(floor(ltime * num_particles - i));
						float ptime2 = float(part / float(num_particles));
						float ptime = float(beat * 0.25 + part * 135);
						Vector3 pos = getCubePos(part / float(num_particles));
						Vector3 temp(cos(ptime) - sin(ptime * 0.8), sin(ptime), sin(ptime + 20));
						float woom = math::notRandf(part);
						pos += normalize(temp) * (1 + woom) * 2;
						float size = 0.3f / (1 + abs(ptime2 - ltime));// 0.3f / (1 + woom);
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

				if (tunnel_particles) {
					float dtime = sync_get_val(cameraTimeTrack, row) / 16;

					device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
					device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
					particle_fx->setTexture("tex", particle_tex);
					particle_fx->setFloat("alpha", pow(1.0f, 2.2f));
					particleStreamer.begin();
					const int num_particles = 5000;
					for (int i = 0; i < num_particles; ++i) {
						Vector3 pos(math::notRandf(i) - 0.5, math::notRandf(i+1) - 0.5, math::notRandf(i+2) - 0.5);
						pos *= 80;
						float woom = math::notRandf(part);
						float size = 0.4f / (1 + math::notRandf(i+3));
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

				device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
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
