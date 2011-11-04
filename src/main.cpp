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

		RenderTexture color_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);
		RenderTexture depth_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_R32F);
		Surface depthstencil = device.createDepthStencilSurface(letterbox_viewport.Width, letterbox_viewport.Height, D3DFMT_D24S8);

		RenderTexture dof_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);
		RenderTexture dof_temp1_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);
		RenderTexture dof_temp2_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);

		/** DEMO ***/

		Effect *dof_fx      = engine::loadEffect(device, "data/dof.fx");
		Effect *blur_fx      = engine::loadEffect(device, "data/blur.fx");
		Effect *color_map_fx = engine::loadEffect(device, "data/color_map.fx");
		dof_fx->setVector3("viewport", Vector3(letterbox_viewport.Width, letterbox_viewport.Height, 0.0f));

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

		Mesh *cube_room_x = engine::loadMesh(device, "data/cube-room.x");
		Effect *cube_room_fx = engine::loadEffect(device, "data/cube-room.fx");
		Texture cube_room_ao_tex = engine::loadTexture(device, "data/cube-room-ao.png");
		Texture cube_room_diff_tex = engine::loadTexture(device, "data/cube-room-diff.png");
		Texture cube_room_norm_tex = engine::loadTexture(device, "data/cube-room-norm.png");
		Texture cube_room_spec_tex = engine::loadTexture(device, "data/cube-room-spec.png");
		cube_room_fx->setTexture("ao_tex", cube_room_ao_tex);
		cube_room_fx->setTexture("diff_tex", cube_room_diff_tex);
		cube_room_fx->setTexture("norm_tex", cube_room_norm_tex);
		cube_room_fx->setTexture("spec_tex", cube_room_spec_tex);

#if 0
		DWORD FVF = cube_room_x->p->GetFVF();
		if (1 && FVF & D3DFVF_NORMAL) {
			FVF |= D3DFVF_TEX3 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE3(1) | D3DFVF_TEXCOORDSIZE3(2);
			LPD3DXMESH tmp_mesh;
			cube_room_x->p->CloneMeshFVF(cube_room_x->p->GetOptions(), FVF, device, &tmp_mesh);
			cube_room_x->attachRef(tmp_mesh);
			DWORD *ad1 = new DWORD[cube_room_x->p->GetNumFaces() * sizeof(DWORD) * 3];
			cube_room_x->p->GenerateAdjacency(1e-6f, ad1);
//			D3DXComputeNormals(cube_room_x->p, ad1);
#if 1
			D3DXComputeTangentFrameEx(cube_room_x->p,
				D3DDECLUSAGE_TEXCOORD, 0,
				D3DDECLUSAGE_TEXCOORD, 1,
				D3DDECLUSAGE_TEXCOORD, 2,
				D3DDECLUSAGE_NORMAL, 0,
				D3DXTANGENT_GENERATE_IN_PLACE | D3DXTANGENT_CALCULATE_NORMALS,
				ad1, 2.0f, 0.0f, 2.0f,
				NULL, NULL);
#else
//			D3DXComputeTangent(cube_room_x->p, 0, 1, 2, 1, ad1);
			D3DXComputeTangentFrame(cube_room_x->p, D3DXTANGENT_GENERATE_IN_PLACE);
#endif
			delete [] ad1;
//			D3DXSaveMeshToX("data/cube-room.x", cube_room_x->p, NULL, NULL, NULL, 0, D3DXF_FILEFORMAT_BINARY | D3DXF_FILEFORMAT_COMPRESSED);
		}
#endif
#if 0
		if (1) {
			NVMeshMender aMender;
			cube_room_x->p->LockVertexBuffer(D
			for (int i = 0; i < cube_room_x->p->GetNumVertices(); ++i)
			std::vector<NVMeshMender::VertexAttribute> inputAtts; // What you have
			std::vector<NVMeshMender::VertexAttribute> outputAtts; // What you want.

			NVMeshMender::VertexAttribute posAtt;
			posAtt.Name_ = "position";
			posAtt.floatVector_ = vpos;

		}
#endif

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
			sync_update(rocket, int(row), &bass_cb, (void *)stream);
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

			float fog_density = sync_get_val(fogDensityTrack, row) / 100000;

			Vector3 worldLightPosition = Vector3(0, sin(beat * 0.25) * 100, 0);
//			Vector3 worldLightPosition = Vector3(0, 0, 0);
			Vector3 viewLightPosition = mul(view, worldLightPosition);
			cube_room_fx->setVector3("viewLightPosition", viewLightPosition);

			for (int i = -1; i < 2; ++i)
				for (int j = -1; j < 2; ++j) {
					Matrix4x4 world = Matrix4x4::translation(Vector3(i * 120, 0, j * 120));

					cube_room_fx->setMatrices(world, view, proj);
					cube_room_fx->commitChanges();
					cube_room_fx->draw(cube_room_x);
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

				particle_fx->setVector3("up", up);
				particle_fx->setVector3("left", left);
				particle_fx->setFloat("alpha", pow(1.0f, 2.2f));
				particle_fx->setMatrices(world, view, proj);

				particleStreamer.begin();
#if 0
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
#endif
				particleStreamer.add(worldLightPosition, 10.0f);
				particleStreamer.end();
				particle_fx->draw(&particleStreamer);

				device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
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
			dof_fx->setFloat("focal_distance", 100);
			dof_fx->setFloat("focal_length", 10);
			dof_fx->setFloat("f_stop", 25);
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
			color_map_fx->setVector3("noffs", Vector3(math::notRandf(int(beat * 100)), math::notRandf(int(beat * 100) + 1), 0));
			color_map_fx->setFloat("flash", flash < 0 ? math::randf() : pow(flash, 2.0f));
			color_map_fx->setFloat("fade", pow(fade, 2.2f));
			color_map_fx->setFloat("bloom_amt", sync_get_val(bloomAmtTrack, row));
			color_map_fx->setFloat("blur_amt", sync_get_val(colorMapBlurTrack, row));
			color_map_fx->setFloat("noise_amt", pow(sync_get_val(colorMapNoiseTrack, row) / 255, 2.2f));
			color_map_fx->setFloat("dist_amt", sync_get_val(distAmtTrack, row));
			color_map_fx->setFloat("dist_freq", sync_get_val(distFreqTrack, row) * 2 * float(M_PI));
			color_map_fx->setFloat("dist_time", float(beat * 4) + sync_get_val(distOffsetTrack, row));
			color_map_fx->setTexture("bloom", dof_target);
			color_map_fx->setTexture("tex", dof_target);
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
