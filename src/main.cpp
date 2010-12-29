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
using renderer::RenderTexture;

using engine::Mesh;
using engine::Effect;
using engine::Anim;

using namespace core;

void makeLetterboxViewport(D3DVIEWPORT9 *viewport, int screen_width, int screen_height, float screen_aspect, float demo_aspect)
{
	int letterbox_width, letterbox_height;

	if (demo_aspect > screen_aspect) {
		/* demo is wider than screen, letterbox */
		float aspect_change = screen_aspect / demo_aspect;
		letterbox_width  = screen_width;
		letterbox_height = int(math::round(screen_height * aspect_change));
	} else {
		/* screen is wider than demo, pillarbox */
		float aspect_change = demo_aspect / screen_aspect;
		letterbox_width  = int(math::round(screen_width * aspect_change));
		letterbox_height = screen_height;
	}

	viewport->X = (screen_width  - letterbox_width) / 2;
	viewport->Y = (screen_height - letterbox_height) / 2;

	viewport->Width  = letterbox_width;
	viewport->Height = letterbox_height;
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
		stream = BASS_StreamCreateFile(false, "data/tune.ogg", 0, 0, BASS_MP3_SETPOS | BASS_STREAM_PRESCAN | ((0 == config::soundcard) ? BASS_STREAM_DECODE : 0));
		if (!stream)
			throw FatalException("failed to open tune");

		sync_device *rocket = sync_create_device("data/sync");
		if (!rocket)
			throw FatalException("something went wrong - failed to connect to host?");

#ifndef SYNC_PLAYER
		sync_set_callbacks(rocket, &bass_cb, (void *)stream);
		if (sync_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
			throw FatalException("failed to connect to host");
#endif

		const sync_track *cameraDistanceTrack = sync_get_track(rocket, "cam.dist");
		const sync_track *cameraYTrack        = sync_get_track(rocket, "cam.y");
		const sync_track *cameraRollTrack     = sync_get_track(rocket, "cam.roll");
		const sync_track *cameraOffsetTrack   = sync_get_track(rocket, "cam.offset");
		const sync_track *cameraIndexTrack    = sync_get_track(rocket, "cam.index");
		const sync_track *cameraShakeAmtTrack = sync_get_track(rocket, "cam.shake.amt");
		const sync_track *cameraShakeSpeedTrack = sync_get_track(rocket, "cam.shake.speed");

		const sync_track *colorMapFadeTrack   = sync_get_track(rocket, "cm.fade");
		const sync_track *colorMapFlashTrack  = sync_get_track(rocket, "cm.flash");
		const sync_track *colorMapBlurTrack   = sync_get_track(rocket, "cm.blur");
		const sync_track *colorMapNoiseTrack  = sync_get_track(rocket, "cm.noise");
		const sync_track *colorMapOverlayTrack = sync_get_track(rocket, "cm.overlay");
		const sync_track *colorMapScrollTrack = sync_get_track(rocket, "scroll");
		const sync_track *pulseAmt2Track      = sync_get_track(rocket, "cm.pulse.amt");
		const sync_track *pulseSpeed2Track    = sync_get_track(rocket, "cm.pulse.speed");
		const sync_track *bloomSizeTrack      = sync_get_track(rocket, "bloom.size");
		const sync_track *bloomAmtTrack       = sync_get_track(rocket, "bloom.amt");

		const sync_track *lokingFrame1Track    = sync_get_track(rocket, "loking.frame1");
		const sync_track *lokingAlpha1Track    = sync_get_track(rocket, "loking.alpha1");
		const sync_track *lokingFrame2Track    = sync_get_track(rocket, "loking.frame2");
		const sync_track *lokingAlpha2Track    = sync_get_track(rocket, "loking.alpha2");

		const sync_track *fogDensityTrack     = sync_get_track(rocket, "fog.density");

		const sync_track *light1IndexTrack    = sync_get_track(rocket, "light1.index");
		const sync_track *light1AlphaTrack    = sync_get_track(rocket, "light1.alpha");
		const sync_track *light2IndexTrack    = sync_get_track(rocket, "light2.index");
		const sync_track *light2AlphaTrack    = sync_get_track(rocket, "light2.alpha");

		const sync_track *lineSpeedTrack      = sync_get_track(rocket, "line.speed");
		const sync_track *lineOffsetTrack     = sync_get_track(rocket, "line.offset");
		const sync_track *lineAmtTrack        = sync_get_track(rocket, "line.amt");
		const sync_track *radialAmtTrack      = sync_get_track(rocket, "radial.amt");
		const sync_track *radialAmt2Track      = sync_get_track(rocket, "radial.amt2");
		const sync_track *pulseSpeedTrack     = sync_get_track(rocket, "pulse.speed");
		const sync_track *pulseAmtTrack       = sync_get_track(rocket, "pulse.amt");

		const sync_track *distAmtTrack        = sync_get_track(rocket, "dist.amt");
		const sync_track *distFreqTrack       = sync_get_track(rocket, "dist.freq");
		const sync_track *distOffsetTrack       = sync_get_track(rocket, "dist.offset");

		Surface backbuffer   = device.getRenderTarget(0);

		D3DCAPS9 caps;
		direct3d->GetDeviceCaps(config::adapter, D3DDEVTYPE_HAL, &caps);

		bool use_sm20_codepath = false;
		if (FAILED(direct3d->CheckDeviceFormat(config::adapter, D3DDEVTYPE_HAL, config::mode.Format, D3DUSAGE_QUERY_FILTER | D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)) ||
			caps.PixelShaderVersion < D3DVS_VERSION(3, 0))
			use_sm20_codepath = true;

//		D3DFORMAT render_fmt = use_sm20_codepath ? D3DFMT_A2R10G10B10 : D3DFMT_A16B16G16R16F;
		D3DFORMAT render_fmt = use_sm20_codepath ? D3DFMT_A8R8G8B8 : D3DFMT_A16B16G16R16F;

		RenderTexture color_msaa(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, render_fmt,
		    use_sm20_codepath ? D3DMULTISAMPLE_NONE : config::multisample);
		Surface depthstencil_msaa = device.createDepthStencilSurface(letterbox_viewport.Width, letterbox_viewport.Height, D3DFMT_D24S8,
		    use_sm20_codepath ? D3DMULTISAMPLE_NONE : config::multisample);

		/** DEMO ***/
		RenderTexture cube_light_tex(device, 128, 128, 1, render_fmt);
		std::vector<RenderTexture> color1_hdr, color2_hdr;
		int w = letterbox_viewport.Width, h = letterbox_viewport.Height;
		D3DFORMAT fmt = use_sm20_codepath ? D3DFMT_A2R10G10B10 : D3DFMT_A16B16G16R16F;
		for (int i = 0; w > 0 || h > 0; ++i, w /= 2, h /= 2) {
			color1_hdr.push_back(RenderTexture(device, std::max(w, 1), std::max(h, 1), 1, fmt));
			color2_hdr.push_back(RenderTexture(device, std::max(w, 1), std::max(h, 1), 1, fmt));
		}

		Effect *blur_fx      = engine::loadEffect(device, "data/blur.fx");
		Effect *color_map_fx = engine::loadEffect(device, "data/color_map.fx");

		Texture scroller_tex = engine::loadTexture(device, "data/scroller.png");
		color_map_fx->setTexture("scroller_tex", scroller_tex);

		Texture noise_tex = engine::loadTexture(device, "data/noise.png");
		color_map_fx->setTexture("noise_tex", noise_tex);

		color_map_fx->setVector3("nscale", Vector3(letterbox_viewport.Width / 128.0, letterbox_viewport.Height / 128.0, 0.0f));
		if (use_sm20_codepath)
			color_map_fx->p->SetTechnique("rgbe");

		engine::ParticleStreamer particleStreamer(device);
		Effect *particle_fx = engine::loadEffect(device, "data/particle.fx");
		Texture particle_tex = engine::loadTexture(device, "data/particle.png");
		particle_fx->setTexture("tex", particle_tex);

		Effect *cube_light_fx = engine::loadEffect(device, "data/cube-light.fx");
		cube_light_fx->setTexture("noise_tex", noise_tex);

		Texture cos_tex = device.createTexture(128, 1, 0, 0, D3DFMT_L16, D3DPOOL_MANAGED);
		{
			D3DLOCKED_RECT rect;
			d3dErr(cos_tex.tex->LockRect(0, &rect, NULL, 0));
			for (int i = 0; i < 128; ++i) {
				double th = (i + 0.5) * ((2 * M_PI) / 128.0);
				unsigned short v = (unsigned short)(32767.5 + cos(th) * 32767.5);
				((unsigned short*)rect.pBits)[i] = v;
			}
			d3dErr(cos_tex.tex->UnlockRect(0));
		}
		cube_light_fx->setTexture("cos_tex", cos_tex);

		Mesh *cube_tops_x  = engine::loadMesh(device, "data/cube-grid-tops-32.x");
		Mesh *cube_sides_x = engine::loadMesh(device, "data/cube-grid-sides-32.x");
		Mesh *cube_floor_x = engine::loadMesh(device, "data/cube-grid-floor.x");
		Effect *cube_tops_fx = engine::loadEffect(device, "data/cube-grid-tops.fx");
		Effect *cube_sides_fx = engine::loadEffect(device, "data/cube-grid-sides.fx");
		Effect *cube_floor_fx = engine::loadEffect(device, "data/cube-grid-floor.fx");
		cube_tops_fx->setTexture("cube_light_tex", cube_light_tex);
		cube_sides_fx->setTexture("cube_light_tex", cube_light_tex);
		cube_floor_fx->setTexture("cube_light_tex", cube_light_tex);

		if (use_sm20_codepath) {
			cube_tops_fx->p->SetTechnique("rgbe");
			cube_sides_fx->p->SetTechnique("rgbe");
			cube_floor_fx->p->SetTechnique("rgbe");
		}

		Texture cube_sides_n1_tex = engine::loadTexture(device, "data/cube-grid-sides-n1.dds");
		Texture cube_sides_n2_tex = engine::loadTexture(device, "data/cube-grid-sides-n2.dds");
		Texture cube_sides_n3_tex = engine::loadTexture(device, "data/cube-grid-sides-n3.dds");
		Texture cube_sides_f_tex = engine::loadTexture(device, "data/cube-grid-sides-f.dds");
		Texture cube_sides_ao_tex = engine::loadTexture(device, "data/cube-grid-sides-ao.dds");
		cube_sides_fx->setTexture("n1_tex", cube_sides_n1_tex);
		cube_sides_fx->setTexture("n2_tex", cube_sides_n2_tex);
		cube_sides_fx->setTexture("n3_tex", cube_sides_n3_tex);
		cube_sides_fx->setTexture("f_tex", cube_sides_f_tex);
		cube_sides_fx->setTexture("ao_tex", cube_sides_ao_tex);

		Texture cube_floor_ao_tex = engine::loadTexture(device, "data/cube-grid-floor-ao.dds");
		Texture cube_floor_l_tex = engine::loadTexture(device, "data/cube-grid-floor-l.dds");
		cube_floor_fx->setTexture("ao_tex", cube_floor_ao_tex);
		cube_floor_fx->setTexture("l_tex", cube_floor_l_tex);

		Anim lights = engine::loadAnim(device, "data/lights");
		Anim loking = engine::loadAnim(device, "data/loking");
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
			sync_update(rocket, int(row)); //gets current timing info from the SyncTimer.
#endif
			double beat = row / 4;

			float camTime = float(beat / 4) + sync_get_val(cameraOffsetTrack, row);
			Vector3 camPos, camTarget;
			switch ((int)sync_get_val(cameraIndexTrack, row)) {
			case 0:
				camPos = Vector3(sin(camTime * 0.25f) * 1.5f,
					1,
					cos(camTime * 0.33f) * 1.5f) * sync_get_val(cameraDistanceTrack, row);
				camTarget = Vector3(0, 0, 0);
				break;
			case 1:
				camPos = Vector3((math::frac(4 * camTime / 128) - 0.5f) * 16 * 128,
					sync_get_val(cameraDistanceTrack, row),
					0);
				camTarget = camPos + Vector3(1, sync_get_val(cameraYTrack, row), 0);
				break;
			case 2:
				camPos = Vector3(sin(camTime / 4) * sync_get_val(cameraDistanceTrack, row),
					sync_get_val(cameraYTrack, row),
					cos(camTime / 4) * sync_get_val(cameraDistanceTrack, row));
				camTarget = Vector3(0, 0, 0);
				break;
			default:
				camPos = Vector3(0, 1, 0) * sync_get_val(cameraDistanceTrack, row);
				camTarget = Vector3(0, 0, 0);
			}

			double shake_phase = beat * 32 * sync_get_val(cameraShakeSpeedTrack, row);
			Vector3 camOffs(sin(shake_phase), cos(shake_phase * 0.9), sin(shake_phase - 0.5));
			camPos += camOffs * sync_get_val(cameraShakeAmtTrack, row);
			camTarget += camOffs * sync_get_val(cameraShakeAmtTrack, row);

			float camRoll = sync_get_val(cameraRollTrack, row) * float(2 * M_PI);
			Matrix4x4 view  = Matrix4x4::lookAt(camPos, camTarget, camRoll);
			Matrix4x4 world = Matrix4x4::identity();
			Matrix4x4 proj  = Matrix4x4::projection(60.0f, float(DEMO_ASPECT), 1.0f, 10000.f);

			// render
			device->BeginScene();
			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			device.setRenderTarget(cube_light_tex.getRenderTarget());
			cube_light_fx->setFloat("time", float(beat) * sync_get_val(lineSpeedTrack, row) + sync_get_val(lineOffsetTrack, row));
			cube_light_fx->setFloat("pulse_phase", float((beat / 16) * sync_get_val(pulseSpeedTrack, row)));
			cube_light_fx->setFloat("pulse_amt", sync_get_val(pulseAmtTrack, row));
			cube_light_fx->setFloat("line_amt", sync_get_val(lineAmtTrack, row) * 0.3f);
			cube_light_fx->setFloat("radial_amt", sync_get_val(radialAmtTrack, row));
			cube_light_fx->setFloat("radial_amt2", sync_get_val(radialAmt2Track, row));

			cube_light_fx->setTexture("light1_tex", lights.getTexture((int)sync_get_val(light1IndexTrack, row) % lights.getTextureCount()));
			cube_light_fx->setTexture("light2_tex", lights.getTexture((int)sync_get_val(light2IndexTrack, row) % lights.getTextureCount()));
			cube_light_fx->setFloat("light1_alpha", pow(sync_get_val(light1AlphaTrack, row), 2.2f));
			cube_light_fx->setFloat("light2_alpha", pow(sync_get_val(light2AlphaTrack, row), 2.2f));
			drawQuad(device, cube_light_fx, -1, -1, 2, 2);

			device.setRenderTarget(color_msaa.getRenderTarget());
			device.setDepthStencilSurface(depthstencil_msaa);
			device->SetRenderState(D3DRS_ZENABLE, true);

			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			device->SetRenderState(D3DRS_ZWRITEENABLE, true);
			device->Clear(0, 0, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0xFF000000, 1.f, 0);

			float fog_density = sync_get_val(fogDensityTrack, row) / 100000;

			for (int i = 0; i < 4; ++i)
				for (int j = 0; j < 4; ++j) {
					Vector3 uv_offs = Vector3(i / 4.0f, j / 4.0f, 0);
					Matrix4x4 world = Matrix4x4::translation(Vector3((i - 2) * 512, 0, (j - 2) * 512));

					cube_tops_fx->setMatrices(world, view, proj);
					cube_tops_fx->setFloat("fog_density", fog_density);
					cube_tops_fx->setVector3("uv_offs", uv_offs);
					cube_tops_fx->commitChanges();
					cube_tops_fx->draw(cube_tops_x);

					cube_sides_fx->setMatrices(world, view, proj);
					cube_sides_fx->setFloat("fog_density", fog_density);
					cube_sides_fx->setVector3("uv_offs", uv_offs);
					cube_sides_fx->commitChanges();
					cube_sides_fx->draw(cube_sides_x);

					cube_floor_fx->setMatrices(world, view, proj);
					cube_floor_fx->setFloat("fog_density", fog_density);
					cube_floor_fx->setVector3("uv_offs", uv_offs);
					cube_floor_fx->commitChanges();
					cube_floor_fx->draw(cube_floor_x);
				}

			if (!use_sm20_codepath) {
				Matrix4x4 modelview = world * view;
				Vector3 up(modelview._12, modelview._22, modelview._32);
				Vector3 left(modelview._11, modelview._21, modelview._31);
				math::normalize(up);
				math::normalize(left);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

				particle_fx->setFloatArray("up", up, 3);
				particle_fx->setFloatArray("left", left, 3);
				particle_fx->setFloat("alpha", pow(0.25f, 2.2f));
				particle_fx->setMatrices(world, view, proj);

				particleStreamer.begin();
				for (int i = 0; i < 60 * 1024; ++i) {
					Vector3 pos(
						(       math::notRandf(i * 4) - 0.5f) * 16 * 64,
						9 + pow(math::notRandf(i * 4 + 1), 2) * 300,
						(       math::notRandf(i * 4 + 2) - 0.5f) * 16 * 64);
					float size = 0.4f + math::notRandf(i * 4 + 1) * 0.5f;
	//				if (distance(pos, camPos) < 200)
					Vector3 temp = pos - camPos;
					if (dot(temp, temp) < 400 * 400)
						particleStreamer.add(pos, size);
					if (!particleStreamer.getRoom()) {
						particleStreamer.end();
						particle_fx->draw(&particleStreamer);
						particleStreamer.begin();
					}
				}
				particleStreamer.end();
				particle_fx->draw(&particleStreamer);

				device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			}
			color_msaa.resolve(device);

			world.makeIdentity();
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			device->SetRenderState(D3DRS_ZWRITEENABLE, false);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

			if (use_sm20_codepath) {
				blur_fx->p->SetTechnique("hack");
				blur_fx->setTexture("blur_tex", color_msaa);

				device.setRenderTarget(color1_hdr[0].getSurface(), 0);
				int w = color_msaa.getWidth();
				int h = color_msaa.getHeight();
				drawRect(device, blur_fx, 0, 0, float(w), float(h));

				blur_fx->p->SetTechnique("blur");
			} else
				device->StretchRect(color_msaa.getSurface(0), NULL, color1_hdr[0].getSurface(), NULL, D3DTEXF_LINEAR);

			/* downsample until bloom fits */
			float stdDev = std::max(sync_get_val(bloomSizeTrack, row), 0.0f) * (letterbox_viewport.Width / (1280.0f / 2));
			float flevel = ::log(stdDev * 3 / 16) / ::log(2.0f);
			flevel = std::max(flevel, 0.0f);
			flevel = std::min(flevel, color1_hdr.size() - 1.0f);
			int level = int(ceil(flevel));
			for (int i = 0; i < level; ++i) {
				d3dErr(device->StretchRect(color1_hdr[i].getSurface(0), NULL, color1_hdr[i + 1].getSurface(0), NULL, D3DTEXF_LINEAR));
				stdDev /= 2;
			}
			assert(3 * stdDev <= 16.0f);

			/* do the bloom */
			RenderTexture render_textures[2] = { color1_hdr[level], color2_hdr[level] };
			device->SetDepthStencilSurface(NULL);
			device->SetRenderState(D3DRS_ZENABLE, false);

			int rtIndex = 0;
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
				int size = int(ceil(3 * stdDev / 2));
				for (int k = 1; k < size; ++k) {
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
						gauss[k].x = o / render_textures[rtIndex].getWidth();
						gauss[k].y = 0.0f;
					} else {
						gauss[k].x = 0.0f;
						gauss[k].y = o / render_textures[rtIndex].getHeight();
					}
					gauss[k].w = 0.0f;
					total += 2 * w;
				}

				for (int k = 0; k < size; ++k)
					gauss[k].z /= total;

				blur_fx->p->SetVectorArray("gauss", gauss, size);
				blur_fx->setTexture("blur_tex", render_textures[rtIndex]);
				blur_fx->p->SetInt("size", size);

				device.setRenderTarget(render_textures[!rtIndex].getSurface(0), 0);
				int w = render_textures[rtIndex].getSurface(0).getWidth();
				int h = render_textures[rtIndex].getSurface(0).getHeight();
				drawRect(device, blur_fx, 0, 0, float(w), float(h));
				rtIndex = !rtIndex;
			}

			/* letterbox */
			device.setRenderTarget(backbuffer);
			device->SetDepthStencilSurface(NULL);
			device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.f, 0);
			device.setViewport(&letterbox_viewport);

			float flash = sync_get_val(colorMapFlashTrack, row);
			float fade = sync_get_val(colorMapFadeTrack, row);
			float pulse = sync_get_val(pulseAmt2Track, row);
			fade = std::max(0.0f, fade - pulse + float(cos(beat * sync_get_val(pulseSpeed2Track, row) * M_PI)) * pulse);
			color_map_fx->setVector3("noffs", Vector3(math::notRandf(int(beat * 100)), math::notRandf(int(beat * 100) + 1), 0));
			color_map_fx->setFloat("flash", flash < 0 ? math::randf() : pow(flash, 2.0f));
			color_map_fx->setFloat("fade", pow(fade, 2.2f));
			color_map_fx->setFloat("scroll", sync_get_val(colorMapScrollTrack, row) / 100.0f);
			color_map_fx->setFloat("bloom_amt", sync_get_val(bloomAmtTrack, row));
			color_map_fx->setFloat("blur_amt", sync_get_val(colorMapBlurTrack, row));
			color_map_fx->setFloat("noise_amt", pow(sync_get_val(colorMapNoiseTrack, row) / 255, 2.2f));
			color_map_fx->setFloat("dist_amt", sync_get_val(distAmtTrack, row));
			color_map_fx->setFloat("dist_freq", sync_get_val(distFreqTrack, row) * 2 * float(M_PI));
			color_map_fx->setFloat("dist_time", float(beat * 4) + sync_get_val(distOffsetTrack, row));
			color_map_fx->setTexture("bloom", color1_hdr[level]);
			color_map_fx->setTexture("tex", color_msaa);
			color_map_fx->setTexture("overlay_tex", overlays.getTexture((int)sync_get_val(colorMapOverlayTrack, row) % overlays.getTextureCount()));
			color_map_fx->setTexture("loking1_tex", loking.getTexture((int)sync_get_val(lokingFrame1Track, row)));
			color_map_fx->setTexture("loking2_tex", loking.getTexture((int)sync_get_val(lokingFrame2Track, row)));
			color_map_fx->setFloat("loking1_alpha", sync_get_val(lokingAlpha1Track, row));
			color_map_fx->setFloat("loking2_alpha", sync_get_val(lokingAlpha2Track, row));
			color_map_fx->commitChanges();


			device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
			drawRect(device, color_map_fx, float(letterbox_viewport.X), float(letterbox_viewport.Y), float(letterbox_viewport.Width), float(letterbox_viewport.Height));
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
