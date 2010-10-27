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

#include "scenegraph/sceneloader.h"

Matrix4x4 radialblur_matrix(const Texture &tex, const Vector2 &center, const float amt = 1.01)
{
	Matrix4x4 trans1;
	trans1.makeIdentity();
	trans1._13 = 0.5f - center.x / 2;
	trans1._23 = 0.5f - center.y / 2;
	
	Matrix4x4 mat;
	mat.makeScaling(Vector3(amt, amt, 1));
	
	Matrix4x4 trans2;
	trans2.makeIdentity();
	trans2._13 = -(0.5f - center.x / 2);
	trans2._23 = -(0.5f - center.y / 2);
	
	return trans1 * mat * trans2;
}

#include "sync/device.h"
#include "sync/basstimer.h"


using namespace core;
using namespace scenegraph;

using sync::Track;

HWND win = 0;
HSTREAM stream = 0;

float randf()
{
	return rand() * (1.f / RAND_MAX);
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

#include "engine/vertexstreamer.h"
void drawFuzz(Effect *effect, engine::VertexStreamer &streamer, float time, float hardness, float xdist_amt, float ydist_amt, float s_nudge = 0.0f, float t_nudge = 0.0f)
{
	UINT passes;
	(*effect)->Begin(&passes, 0);
	for (UINT pass = 0; pass < passes; ++pass)
	{
		(*effect)->BeginPass( pass );
		const int SEGS = 235;
		streamer.begin(D3DPT_TRIANGLELIST);
		float last_xoffs = 0.f;
		float last_yoffs = 0.f;
		for (unsigned i = 0; i < SEGS; ++i)
		{
			float y1 = float(i) * (1.f / SEGS);
			float y2 = y1 + (1.f / SEGS);
			
			float xoffs = 0.0f;
//			xoffs = (randf() - 0.5f) * 0.75f;
			xoffs += sinf(y1 * float(M_PI) + time);
			xoffs += sinf(y1 * 4.2f * float(M_PI) + time) * 0.5f;
			xoffs += fmod(tan(cos(y1) * 18.7f * float(M_PI) + time), 0.9f) * 0.25f * hardness;
			xoffs *= 0.05f * xdist_amt;

			float yoffs = 0.0f;
			yoffs += fmodf(tanf(y1 * float(M_PI) * 4 + time), 0.9f) * hardness;
			yoffs *= 0.05f * ydist_amt;

			streamer.uv(    D3DXVECTOR2(0.f + last_xoffs + s_nudge, 1 - y1 + last_yoffs + t_nudge));
			streamer.vertex(D3DXVECTOR3(-1, (y1 * 2) - 1, 0));
			
			streamer.uv(    D3DXVECTOR2( 0.f + xoffs + s_nudge, 1 - y2 + yoffs + t_nudge));
			streamer.vertex(D3DXVECTOR3(-1, (y2 * 2) - 1, 0));
			
			streamer.uv(    D3DXVECTOR2( 1.f + xoffs + s_nudge, 1 - y2 + yoffs + t_nudge));
			streamer.vertex(D3DXVECTOR3( 1, (y2 * 2) - 1, 0));
			
			streamer.uv(    D3DXVECTOR2( 0 + last_xoffs + s_nudge, 1 - y1 + last_yoffs + t_nudge));
			streamer.vertex(D3DXVECTOR3(-1, (y1 * 2) - 1, 0));
			
			streamer.uv(    D3DXVECTOR2( 1 + xoffs + s_nudge, 1 - y2 + yoffs + t_nudge));
			streamer.vertex(D3DXVECTOR3( 1, (y2 * 2) - 1, 0));
			
			streamer.uv(    D3DXVECTOR2( 1 + last_xoffs + s_nudge, 1 - y1 + last_yoffs + t_nudge));
			streamer.vertex(D3DXVECTOR3( 1, (y1 * 2) - 1, 0));
			last_xoffs = xoffs;
			last_yoffs = yoffs;
		}
		streamer.end();
		(*effect)->EndPass();
	}
	(*effect)->End();
}

Surface loadSurface(renderer::Device &device, std::string fileName)
{
	D3DXIMAGE_INFO srcInfo;
	core::d3dErr(D3DXGetImageInfoFromFile(fileName.c_str(), &srcInfo));
	
	IDirect3DSurface9 *surface = NULL;
	core::d3dErr(device->CreateOffscreenPlainSurface(srcInfo.Width, srcInfo.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL));
	core::d3dErr(D3DXLoadSurfaceFromFile(surface, NULL, NULL, fileName.c_str(), NULL, D3DX_FILTER_NONE, NULL, NULL));
	
	Surface surface_wrapper;
	surface_wrapper.attachRef(surface);
	return surface_wrapper;
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
//	_Module.Init(NULL, hInstance);
	
	try {
		/* create d3d object */
		ComRef<IDirect3D9> direct3d;
		direct3d.attachRef(Direct3DCreate9(D3D_SDK_VERSION));
		if (!direct3d)
			throw FatalException("Your directx-version is from the stone-age.\n\nTHRUG SAYS: UPGRADE!");

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

		/* create window */
		win = CreateWindow("static", "very last engine ever", WS_POPUP, 0, 0, config::mode.Width, config::mode.Height, 0, 0, GetModuleHandle(0), 0);
		if (!win)
			throw FatalException("CreateWindow() failed. something is VERY spooky.");
		
		/* create device */
		Device device;
		device.attachRef(init::initD3D(direct3d, win, config::mode, D3DMULTISAMPLE_NONE, config::adapter, config::vsync));
		
		/* showing window after initing d3d in order to be able to see warnings during init */
		ShowWindow(win, TRUE);
#if !WINDOWED
		ShowCursor(0);
#endif
		
		/* setup letterbox */
		D3DVIEWPORT9 letterbox_viewport = device.getViewport();
		makeLetterboxViewport(&letterbox_viewport, config::mode.Width, config::mode.Height, config::aspect, float(DEMO_ASPECT));

		/* setup sound-playback */
		if (!BASS_Init(config::soundcard, 44100, 0, 0, 0))
			throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/makesnodigital.mp3", 0, 0, BASS_MP3_SETPOS | ((0 == config::soundcard) ? BASS_STREAM_DECODE : 0));
		if (!stream)
			throw FatalException("failed to open tune");

		// setup timer and construct sync-device
		BassTimer synctimer(stream, float(BPM), 4);
		
		std::auto_ptr<sync::Device> syncDevice = std::auto_ptr<sync::Device>(sync::createDevice("data/sync", synctimer));
		if (NULL == syncDevice.get())
			throw FatalException("something went wrong - failed to connect to host?");

		Track &cameraDistanceTrack = syncDevice->getTrack("cam.dist");
		Track &cameraRollTrack     = syncDevice->getTrack("cam.roll");
		Track &cameraXRotTrack     = syncDevice->getTrack("cam.x-rot");
		Track &cameraYRotTrack     = syncDevice->getTrack("cam.y-rot");
		Track &cameraZRotTrack     = syncDevice->getTrack("cam.z-rot");
		Track &cameraOffsetTrack     = syncDevice->getTrack("cam.offset");
		Track &cameraIndexTrack     = syncDevice->getTrack("cam.index");

		Track &colorMapBlendTrack  = syncDevice->getTrack("cm.blend");
		Track &colorMapPalTrack    = syncDevice->getTrack("cm.pal");
		Track &colorMapFadeTrack   = syncDevice->getTrack("cm.fade");
		Track &colorMapFlashTrack  = syncDevice->getTrack("cm.flash");
		Track &colorMapDistortXTrack  = syncDevice->getTrack("cm.dist.x");
		Track &colorMapDistortYTrack  = syncDevice->getTrack("cm.dist.y");
		Track &overlayTrack  = syncDevice->getTrack("cm.overlay");

		Track &noiseAmtTrack  = syncDevice->getTrack("noise.amt");
		Track &noiseFFTTrack  = syncDevice->getTrack("noise.fft");

		Track &partTrack = syncDevice->getTrack("_part");
		Track &logoCycleTrack     = syncDevice->getTrack("logo.cycle");

		Track &starAlphaTrack     = syncDevice->getTrack("star.alpha");
		Track &starRotTrack     = syncDevice->getTrack("star.rot");

//		engine::SpectrumData noise_fft = engine::loadSpectrumData("data/noise.fft");

		Surface backbuffer   = device.getRenderTarget(0);
		Surface depthstencil = device.getDepthStencilSurface();

		D3DCAPS9 caps;
		direct3d->GetDeviceCaps(config::adapter, D3DDEVTYPE_HAL, &caps);

		bool use_sm20_codepath = false;
		if (FAILED(direct3d->CheckDeviceFormat(config::adapter, D3DDEVTYPE_HAL, config::mode.Format, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)) ||
			caps.PixelShaderVersion < D3DVS_VERSION(3, 0))
			use_sm20_codepath = true;

		RenderTexture color_msaa(device, letterbox_viewport.Width, letterbox_viewport.Height, 0,
		    use_sm20_codepath ? D3DFMT_A8R8G8B8 : D3DFMT_A16B16G16R16F,
		    use_sm20_codepath ? D3DMULTISAMPLE_NONE : config::multisample,
		    D3DUSAGE_AUTOGENMIPMAP);
		Surface depthstencil_msaa = device.createDepthStencilSurface(letterbox_viewport.Width, letterbox_viewport.Height, D3DFMT_D24S8,
		    use_sm20_codepath ? D3DMULTISAMPLE_NONE : config::multisample);

		/** DEMO ***/
		RenderTexture color1_hdr(device, 1280 / 4, int((1280 / DEMO_ASPECT) / 4), 1,
		    use_sm20_codepath ? D3DFMT_A8R8G8B8 : D3DFMT_A16B16G16R16F);
		RenderTexture color2_hdr(device, 1280 / 4, int((1280 / DEMO_ASPECT) / 4), 1,
		    use_sm20_codepath ? D3DFMT_A8R8G8B8 : D3DFMT_A16B16G16R16F);

		engine::VertexStreamer vertex_streamer(device);

		scenegraph::Scene *testScene = loadScene(device, "data/testScene/test.scene");
		engine::SceneRenderer testRenderer = engine::SceneRenderer(testScene, testScene->findCamera("Camera01-camera"));

		Effect *tex_fx      = engine::loadEffect(device, "data/tex.fx");
		Effect *tex_trans_fx      = engine::loadEffect(device, "data/tex.fx");
		tex_fx->setMatrix("transform", Matrix4x4::identity());
		Effect *blur_fx     = engine::loadEffect(device, "data/blur.fx");

		Effect *particle_fx = engine::loadEffect(device, "data/particle.fx");
		Texture bartikkel_tex = engine::loadTexture(device, "data/spherenormal.png");
		particle_fx->setTexture("tex", bartikkel_tex);

		Effect *noise_fx = engine::loadEffect(device, "data/noise.fx");
		Texture noise_tex = engine::loadTexture(device, "data/noise.png");

		Texture desaturate_tex = engine::loadTexture(device, "data/desaturate.png");
		Effect *color_map_fx = engine::loadEffect(device, "data/color_map.fx");
		Texture color_maps[2];

		color_maps[0] = engine::loadTexture(device, "data/color_map0.png");
		color_maps[1] = engine::loadTexture(device, "data/color_map1.png");
		color_map_fx->setFloat("texel_width", 1.0f / color_msaa.getWidth());
		color_map_fx->setFloat("texel_height", 1.0f / color_msaa.getHeight());

		engine::ParticleStreamer particleStreamer(device);
		Texture starTexture = engine::loadTexture(device, "data/star.png");
		Effect *starParticleEffect = engine::loadEffect(device, "data/star_particle.fx");
		starParticleEffect->setTexture("tex", starTexture);

		renderer::CubeTexture cubemap_tex = engine::loadCubeTexture(device, "data/diamond-env.dds");

		Effect *skybox_fx = engine::loadEffect(device, "data/skybox.fx");
		skybox_fx->setTexture("reflectionMap", cubemap_tex);
		Mesh *cube_x         = engine::loadMesh(device, "data/cube.X");
		Mesh *hexcol_x         = engine::loadMesh(device, "data/hexcol.X");
		Mesh *boxMesh        = engine::loadMesh(device, "data/box.x");
		Mesh *discoTilesMesh = engine::loadMesh(device, "data/disco_tiles.x");

		renderer::CubeTexture greeble_envmap = engine::loadCubeTexture(device, "data/stpeters_cross3.dds");
		Mesh *greeble_cube_x = engine::loadMesh(device, "data/greeble_cube.x");
		Effect *greeble_cube_fx = engine::loadEffect(device, "data/greeble_cube.fx");
		greeble_cube_fx->setTexture("lightmap", engine::loadTexture(device, "data/greeble_cube_lightmap.png"));
		greeble_cube_fx->setTexture("env", greeble_envmap);
		Effect *black_fx = engine::loadEffect(device, "data/black.fx");

		Anim overlaysAnim = engine::loadAnim(device, "data/overlays/");
		Image overlaysImage(overlaysAnim.getTexture(0), tex_fx);

		Image logoImage(engine::loadTexture(device, "data/komputerpop.png"), tex_fx);

		Effect *logoEffect = greeble_cube_fx; // engine::loadEffect(device, "data/test2.fx");
		Mesh   *logoMesh   = engine::loadMesh(device, "data/logo.x");
		Mesh   *logoRingMesh   = engine::loadMesh(device, "data/logoring.x");
		Effect *logoRingEffect = engine::loadEffect(device, "data/logoring.fx");
		logoRingEffect->setTexture("tex", engine::loadTexture(device, "data/logoring.png"));

		BASS_Start();
		BASS_ChannelPlay(stream, false);
		BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, 0.0f));

		// todo: config this
		bool dump_video = false;
		float video_framerate = 60.0f;

		bool done = false;
		int frame = 0;
		while (!done) {
			if (dump_video)
				BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, frame / video_framerate));

			static float last_time = 0.f;
			static float time_offset = 0.f;
			float time = synctimer.getTime();
			float beat = synctimer.getRow();

#ifndef VJSYS
			syncDevice->update(beat); //gets current timing info from the SyncTimer.
#endif
			device->BeginScene();
			
			/* setup multisampled stuffz */
			device.setRenderTarget(color_msaa.getRenderTarget());
			device.setDepthStencilSurface(depthstencil_msaa);
			
			D3DXCOLOR clear_color(0.45f, 0.25f, 0.25f, 0.f);
			
			float roll = cameraRollTrack.getValue(beat);
			roll *= float(2 * M_PI);
			float yRot = cameraYRotTrack.getValue(beat);
			Vector3 up(float(sin(roll)), float(cos(roll)), 0.f);
			Vector3 eye(
				float(sin(yRot)),
				float(0),
				float(cos(yRot))
			);
			eye = normalize(eye);

			float camera_distance = 360 * (cameraDistanceTrack.getValue(beat));
			eye *= camera_distance;
			Vector3 at(0, 0, 0);

			at = Vector3(0, 0, 0);
			eye = Vector3(0, 0, -4);
			Vector3 orig_at = at;
			
			Matrix4x4 world = Matrix4x4::identity();
			Matrix4x4 view = Matrix4x4::lookAt(eye, at, roll);
			Matrix4x4 proj = Matrix4x4::projection(60.0f, float(DEMO_ASPECT), 1.0f, 100000.f);

//			testScene->anim(fmod(beat, 100));
			scenegraph::Camera *cam = testScene->findCamera("Camera01-camera");
			if (cam) {
				Matrix4x4 camView = cam->getAbsoluteTransform();
				view = camView.inverse();
				proj = cam->getProjection();
			}

			// render
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			device->SetRenderState(D3DRS_ZWRITEENABLE, true);
			device->Clear(0, 0, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, clear_color, 1.f, 0);

			//	test_fx.draw(cube_x);
			//	time /= 4;
/*
			testRenderer.view       = view;
			testRenderer.projection = proj; // math::Matrix4x4::projection(60.0f, 16.0f / 9, 1.0f, 1000.0f);
			testRenderer.draw(); */

			float dist = cameraDistanceTrack.getValue(beat);
			float rot = 0; // cameraRollTrack.getValue(beat);
			int cameraIndex = cameraIndexTrack.getIntValue(beat);

			float camTime = beat / 4 + cameraOffsetTrack.getValue(beat);
			Vector3 camPos;
			switch (cameraIndex) {
			case 0:
				camPos = Vector3(-sin(camTime*0.3f)*280, cos(camTime*0.3f)*80, cos(-camTime * 0.4f + 1)*220);
				break;
			case 1:
				camPos = Vector3(sin(camTime*0.3f)*200, cos(camTime*0.7f)*70, cos(-camTime*0.3f)*160);
				break;
			default:
				camPos = Vector3(sin(camTime * 0.25f) * 200, cos(camTime * 0.7f) * 70, -(120 + (camTime - 8) * 1.f)) * dist;
			}

			view = Matrix4x4::lookAt(camPos, Vector3(0,0,50), roll);
			world = Matrix4x4::rotation(Vector3(0, -M_PI / 2, 0));

			if (partTrack.getIntValue(beat) == 1) {
				for (int j = -2; j < 3; ++j) {
					float dir = j & 1 ? -1.0f : 1.0f;
					for (int i = 0; i < 16; ++i) {
						Matrix4x4 world = Matrix4x4::translation(Vector3(0, 30, 0));
						world *= Matrix4x4::rotation(Vector3(0, 0, (float(i) / 16  + beat / 64) * 2 * M_PI * dir));
						world *= Matrix4x4::translation(Vector3(0, j * 70, 0));

						logoEffect->setMatrices(world, view, proj);
						logoEffect->commitChanges();
						logoEffect->draw(boxMesh);
					}

					for (int i = 0; i < 16; ++i) {
						Matrix4x4 world = Matrix4x4::translation(Vector3(0, 30, 0));
						world *= Matrix4x4::rotation(Vector3(0, 0, (float(i) / 16  + beat / 64) * 2 * M_PI * dir));
						world *= Matrix4x4::rotation(Vector3(M_PI / 2, 0, 0));
						world *= Matrix4x4::translation(Vector3(0, -35 + j * 70, 0));

						logoEffect->setMatrices(world, view, proj);
						logoEffect->commitChanges();
						logoEffect->draw(boxMesh);
					}
				}
			} else if (0 == partTrack.getIntValue(beat)) {
				logoEffect->setMatrices(world, view, proj);
				logoEffect->commitChanges();
				logoEffect->draw(logoMesh);
			} else if (2 == partTrack.getIntValue(beat)) {
				logoEffect->setMatrices(world, view, proj);
				logoEffect->commitChanges();
				logoEffect->draw(discoTilesMesh);
			} else {
				logoEffect->setMatrices(world, view, proj);
				logoEffect->commitChanges();

				for (int y = -16; y < 16; ++y) {
					for (int x = -16; x < 16; ++x) {
						Vector3 center(y * 1.7, 0, x * 2 + y % 2);
						center.y  = cos(center.x * 0.1f - center.z * 0.15f - beat);
						center.y -= sin(center.x * 0.2f - center.z * 0.11f + beat);
						center.y -= 3.0f;
						Matrix4x4 world = Matrix4x4::translation(center) * Matrix4x4::scaling(Vector3(15, 30, 15)) ;
						logoEffect->setMatrices(world, view, proj);
						logoEffect->commitChanges();
						logoEffect->draw(hexcol_x);
					}
				}
			}

			device->SetRenderState(D3DRS_ZWRITEENABLE, false);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			for (int i = 0; i < 3; ++i) {
				Matrix4x4 world = Matrix4x4::rotation(Vector3(0, -M_PI / 2, 0));
				world *= Matrix4x4::scaling(Vector3(i + 3, i + 3, i + 3) * 3);
				world *= Matrix4x4::rotation(Vector3(0, 0, beat + i));

				float color = pow(fmod(logoCycleTrack.getValue(beat + i), 1), 3);
				logoRingEffect->setFloat("alpha", color);
				logoRingEffect->setMatrices(world, view, proj);
				logoRingEffect->commitChanges();
				logoRingEffect->draw(logoRingMesh);
			}

			Matrix4x4 modelview = world * view;
			up = Vector3(modelview._12, modelview._22, modelview._32);
			Vector3 left(modelview._11, modelview._21, modelview._31);
			math::normalize(up);
			math::normalize(left);
			starParticleEffect->setFloatArray("up", up, 3);
			starParticleEffect->setFloatArray("left", left, 3);
			starParticleEffect->setMatrices(world, view, proj);
			starParticleEffect->setFloat("alpha", starAlphaTrack.getValue(beat));
			starParticleEffect->commitChanges();

			if (2 == partTrack.getIntValue(beat)) {
				particleStreamer.begin();
				for (int i = 0; i < 256; ++i) {
#if 1
					int j = i - 8;
					float scale = 1.0f / (1 + abs(j) * 0.5f);
					Vector3 pos = Vector3(cos(float(i) * 350) * 50, cos(float(i) * 150) * 50, fabs(cos((beat + i * 0.1) * (M_PI / 8))) * 100 * scale - 100);
#else
					float rot = (float(i) / 16 + beat / 32) * (2 * M_PI);
					float dist = (float(i) / 16) * 100;
					float scale = float(i) / 16;
					Vector3 pos = Vector3(sin(rot) * dist, 50, -cos(rot) * dist);
#endif
					particleStreamer.add(pos, 25 * scale);
				}
				particleStreamer.end();
			} else {
				particleStreamer.begin();
				for (int i = 0; i < 256; ++i) {
#if 0
					int j = i - 8;
					float scale = 1.0f / (1 + abs(j) * 0.5f);
					Vector3 pos = Vector3(cos(float(i) * 350) * 150, 150 + cos(float(i) * 150) * 50, fabs(cos((beat + i * 0.1) * (M_PI / 8))) * 100 * scale - 50);
#else
					float rot = (float(i) / 16 + beat / 32) * float(2 * M_PI);
					float dist = (float(i) / 16) * 100;
					float scale = float(i) / 16;
					Vector3 pos = Vector3(sin(rot) * dist, 50, -cos(rot) * dist);
#endif
					particleStreamer.add(pos, 25 * scale);
				}
				particleStreamer.end();
			}
			starParticleEffect->draw(&particleStreamer);

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			color_msaa.resolve(device);
#if 1
			world.makeIdentity();
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			device->SetRenderState(D3DRS_ZWRITEENABLE, false);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

			device->StretchRect(color_msaa.getSurface(0), NULL, color1_hdr.getSurface(0), NULL, D3DTEXF_LINEAR);
			blur_fx->setFloat("sub", 0.25f);
			RenderTexture render_textures[2] = { color1_hdr, color2_hdr };
			int rtIndex = 0;
			device->SetDepthStencilSurface(NULL);
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 2; j++) {

					float dir_vec[2];
					float dir = float(M_PI / 2 * j);
					dir_vec[0] = cosf(dir) * (float(1 << i) / render_textures[rtIndex].getWidth());
					dir_vec[1] = sinf(dir) * (float(1 << i) / render_textures[rtIndex].getWidth());
					dir_vec[0] *= 3.f / 4.f;

					device.setRenderTarget(render_textures[!rtIndex].getRenderTarget(), 0);
					blur_fx->setFloatArray("dir", dir_vec, 2);
					blur_fx->setTexture("blur_tex", render_textures[rtIndex]);

					drawQuad(
						device, blur_fx,
						-1.0f, -1.0f,
						 2.0f, 2.0f,
						0.5f / render_textures[!rtIndex].getWidth(),
						0.5f / render_textures[!rtIndex].getHeight()
					);
					blur_fx->setFloat("sub", 0.0f);
					rtIndex = !rtIndex;
				}
			}
#endif

			/* letterbox */
			device.setRenderTarget(backbuffer);
			device->SetDepthStencilSurface(NULL);
			device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.f, 0);
			device.setViewport(&letterbox_viewport);

			float flash = (colorMapFlashTrack.getValue(beat) == 1000.f) ? float(rand() % 2) : colorMapFlashTrack.getValue(beat);
			color_map_fx->setFloat("fade", colorMapBlendTrack.getValue(beat));
			color_map_fx->setFloat("flash", pow(flash, 2.0f));
			color_map_fx->setFloat("fade2", colorMapFadeTrack.getValue(beat));
			color_map_fx->setFloat("alpha", 0.25f);
			color_map_fx->setTexture("tex", color1_hdr);
			color_map_fx->setTexture("tex2", color_msaa);
			color_map_fx->setTexture("color_map", color_maps[0]);
			color_map_fx->setTexture("desaturate", desaturate_tex);

			drawQuad(device, color_map_fx,
			    -1.0f, -1.0f,
			    2.0f, 2.0f,
			    0.5f / backbuffer.getWidth(),
			    0.5f / backbuffer.getHeight());
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

			BASS_Update(); // decrease the chance of missing vsync
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
		}




		/** END OF DEMO ***/

		// cleanup
		if (stream)
			BASS_StreamFree(stream);
		BASS_Free();
		if (win)
			DestroyWindow(win);
#if !WINDOWED
		ShowCursor(TRUE);
#endif
	} catch (const std::exception &e) {
		// cleanup
		if (stream)
			BASS_StreamFree(stream);
		BASS_Free();
		if (win)
			DestroyWindow(win);
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
