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
#include "engine/particlestreamer.h"
#include "engine/particlecloud.h"

#include "engine/explosion.h"

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


WTL::CAppModule _Module;

using namespace core;
using namespace scenegraph;

using sync::Track;

HWND win = 0;
HSTREAM stream = 0;

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
	surface_wrapper.Attach(surface);
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
	
	_Module.Init(NULL, GetModuleHandle(0));
	
	try
	{
		/* create d3d object */
		CComPtr<IDirect3D9> direct3d = com_ptr(Direct3DCreate9(D3D_SDK_VERSION));
		if (!direct3d) throw FatalException("your directx-version is from the stone-age.\n\nTHRUG SAYS: UPGRADE!");
		
		ConfigDialog config(direct3d);
		
#if !WINDOWED
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
		
		if (FAILED(direct3d->CheckDeviceFormat(config.getAdapter(), DEVTYPE, config.getMode().Format, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)))
		{
			MessageBox(NULL, "Selected mode does not support FP16 texture-filtering, demo will look crap.", "visual quality warning", MB_OK | MB_ICONWARNING);
		}
		
		if (FAILED(direct3d->CheckDeviceFormat(config.getAdapter(), DEVTYPE, config.getMode().Format, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)))
		{
			MessageBox(NULL, "Selected mode does not support FP16 blending, demo will look crap.", "visual quality warning", MB_OK | MB_ICONWARNING);
		}
		
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
		makeLetterboxViewport(&letterbox_viewport, config.getWidth(), config.getHeight(), config.getAspect(), float(DEMO_ASPECT));
		
		/* setup sound-playback */
		if (!BASS_Init(config.getSoundcard(), 44100, 0, 0, 0)) throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/soundtrack.ogg", 0, 0, BASS_MP3_SETPOS | ((0 == config.getSoundcard()) ? BASS_STREAM_DECODE : 0));
		if (!stream) throw FatalException("failed to open tune");
		
		// setup timer and construct sync-device
		BassTimer synctimer(stream, BPM, 4);
		
		std::auto_ptr<sync::Device> syncDevice = std::auto_ptr<sync::Device>(sync::createDevice("data/sync", synctimer));
		if (NULL == syncDevice.get()) throw FatalException("something went wrong - failed to connect to host?");
		
		Track &cameraDistanceTrack = syncDevice->getTrack("cam.dist");
		Track &cameraRollTrack     = syncDevice->getTrack("cam.roll");
		Track &cameraYRotTrack     = syncDevice->getTrack("cam.y-rot");
		Track &cameraUpTrack     = syncDevice->getTrack("cam.up");
		Track &cameraShakeAmtTrack     = syncDevice->getTrack("cam.shake.amt");
		Track &cameraShakeTempoTrack     = syncDevice->getTrack("cam.shake.tempo");
		
		Track &colorMapBlendTrack  = syncDevice->getTrack("cm.blend");
		Track &colorMapPalTrack    = syncDevice->getTrack("cm.pal");
		Track &colorMapFadeTrack   = syncDevice->getTrack("cm.fade");
		Track &colorMapFlashTrack  = syncDevice->getTrack("cm.flash");
		Track &colorMapDistortXTrack  = syncDevice->getTrack("cm.dist.x");
		Track &colorMapDistortYTrack  = syncDevice->getTrack("cm.dist.y");
		
		Track &noiseAmtTrack  = syncDevice->getTrack("noise.amt");
		Track &noiseFFTTrack  = syncDevice->getTrack("noise.fft");
		
		sync::Track &explosionTrack = syncDevice->getTrack("explosion");

		engine::SpectrumData noise_fft = engine::loadSpectrumData("data/noise.fft");
		
		Surface backbuffer   = device.getRenderTarget(0);
		Surface depthstencil = device.getDepthStencilSurface();
		
		RenderTexture color_msaa(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8, config.getMultisample());
		Surface depthstencil_msaa = device.createDepthStencilSurface(letterbox_viewport.Width, letterbox_viewport.Height, D3DFMT_D24S8, config.getMultisample());
		
		/** DEMO ***/
		
		RenderTexture color1_hdr(device, 800 / 2, int((800 / DEMO_ASPECT) / 2), 1, D3DFMT_A16B16G16R16F);
		RenderTexture color2_hdr(device, 800 / 2, int((800 / DEMO_ASPECT) / 2), 1, D3DFMT_A16B16G16R16F);

/*		RenderTexture depth(device, 800 / 2, int((800 / DEMO_ASPECT) / 2), 1, D3DFMT_R32F); */

		engine::VertexStreamer vertex_streamer(device);
		
		// load a scene we can render, yes
		scenegraph::Scene *testScene = scenegraph::loadScene(device, "data/test.scene");
		engine::SceneRenderer testRenderer(testScene, NULL);

/*		scenegraph::Node *testNode = testScene->findChild("Camera01-node_transform");
		assert(NULL != testNode); */
		
//		RenderTexture rt(device, 128, 128, 1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE);
//		RenderTexture rt2(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8);
//		RenderTexture rt3(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8);
		
		Effect *tex_fx      = engine::loadEffect(device, "data/tex.fx");
		tex_fx->setMatrix("transform", Matrix4x4::identity());
		Effect *blur_fx     = engine::loadEffect(device, "data/blur.fx");
		
		Effect *particle_fx = engine::loadEffect(device, "data/particle.fx");
		Texture bartikkel_tex = engine::loadTexture(device, "data/spherenormal.png");
		particle_fx->setTexture("tex", bartikkel_tex);
		
		Effect *particle2_fx = engine::loadEffect(device, "data/particle2.fx");
		Texture lightparticle_tex = engine::loadTexture(device, "data/lightparticle.png");
		particle2_fx->setTexture("tex", lightparticle_tex);
		
		Effect *noise_fx = engine::loadEffect(device, "data/noise.fx");
		Texture noise_tex = engine::loadTexture(device, "data/noise.png");
		
		Texture desaturate_tex = engine::loadTexture(device, "data/desaturate.png");
		Effect *color_map_fx = engine::loadEffect(device, "data/color_map.fx");
		Texture color_maps[2];
		
		color_maps[0] = engine::loadTexture(device, "data/color_map0.png");
		color_maps[1] = engine::loadTexture(device, "data/color_map1.png");
		color_map_fx->setFloat("texel_width", 1.0f / color_msaa.getWidth());
		color_map_fx->setFloat("texel_height", 1.0f / color_msaa.getHeight());
		
		Effect *cubegrid_fx = engine::loadEffect(device, "data/cubegrid.fx");
		renderer::VolumeTexture front_tex = engine::loadVolumeTexture(device, "data/front.dds");
		cubegrid_fx->setTexture("front_tex", front_tex);
		
		engine::VoxelGrid voxelGrid = engine::loadVoxelGrid("data/duck.voxel");
		engine::VoxelMesh voxelMesh(device, *cubegrid_fx, voxelGrid, 64);

		engine::ParticleStreamer streamer(device);

		struct ParticleData
		{
			ParticleData(float size, const Vector3 &dir) : size(size), dir(dir) {}
			float size;
			Vector3 dir;
		};
		engine::ParticleCloud<ParticleData> cloud;
		for (int i = 0; i < 12 * 1024; ++i)
		{
			float x, y, z;
			do {
				x = (randf() - 0.5f) * 2;
				z = (randf() - 0.5f) * 2;
				y = (randf() - 0.5f) * 2;
			} while ((x * x + y * y + z * z) > 1.0f || (fabs(x) < 1e-5 && fabs(y) < 1e-5 && fabs(z) < 1e-5));

			float dist = randf();
			dist = pow(dist, 5.0f);
			dist += 0.01f;
			x *= dist;
			y *= dist;
			z *= dist;

			float s = (randf() + 0.5f) * 0.45f;
			cloud.addParticle(
				engine::Particle<ParticleData>(
					Vector3(x, y, z) * 200,
					ParticleData(s,
						math::normalize(Vector3(
						1.0f / x,
						1.0f / y,
						1.0f / z
						))
					)
				)
			);
		}
		engine::ParticleCloud<ParticleData> cloud2;
		for (int i = 0; i < 4 * 1024; ++i)
		{
			float x, y, z;
			do {
				x = (randf() - 0.5f) * 2;
				z = (randf() - 0.5f) * 2;
				y = (randf() - 0.5f) * 2;
			} while ((x * x + y * y + z * z) > 1.0f || (fabs(x) < 1e-5 && fabs(y) < 1e-5 && fabs(z) < 1e-5));
			
			float dist = randf();
			dist = pow(dist, 32.0f);
			dist += 0.01f;
			x *= dist;
			y *= dist;
			z *= dist;
			
			float s = (randf() + 0.75f) * 0.55f;
			cloud2.addParticle(
				engine::Particle<ParticleData>(
					Vector3(x, y, z) * 75,
					ParticleData(s,
						math::normalize(Vector3(
						1.0f / x,
						1.0f / y,
						1.0f / z
						))
					)
				)
			);
		}

		Effect *explosion_fx = engine::loadEffect(device, "data/explosion.fx");
		Texture explosion_tex = engine::loadTexture(device, "data/explosion.png");
		explosion_fx->setTexture("explosion_tex", explosion_tex);
		engine::Explosion explosion = engine::Explosion(device, Vector3(0.1f, 0.1f, 0.1f), Vector3(-2.9f, 0.1f, 2.5f));

		Image scanlinesImage(engine::loadTexture(device, "data/scanlines.png"), tex_fx);
		
		renderer::CubeTexture cubemap_tex = engine::loadCubeTexture(device, "data/stpeters_cross3.dds");
/*
		Effect test_fx = engine::loadEffect(device, "data/test.fx");
		test_fx->SetTexture("env", cube);
*/
		Effect *skybox_fx = engine::loadEffect(device, "data/skybox.fx");
		skybox_fx->setTexture("reflectionMap", cubemap_tex);
		Mesh *cube_x         = engine::loadMesh(device, "data/cube.X");
		
		Effect *tex_trans_fx       = engine::loadEffect(device, "data/tex_trans.fx");
		Texture excess_outline_tex = engine::loadTexture(device, "data/excess_outline.dds");
		Texture demotitle_tex      = engine::loadTexture(device, "data/demotitle.dds");

		Surface logo_surf = loadSurface(device, "data/logo.png");
		Texture bar_tex = engine::loadTexture(device, "data/bar.png");

		Effect *tunelle_fx = engine::loadEffect(device, "data/tunelle.fx");
		tunelle_fx->setTexture("tex", engine::loadTexture(device, "data/tunelle.dds"));
		Mesh *tunelle_x = engine::loadMesh(device, "data/tunelle.x");

		
		BASS_Start();
		BASS_ChannelPlay(stream, false);
		BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, 30.0f));
		
		bool done = false;
		while (!done)
		{
#ifdef DUMP_VIDEO
			static int frame = 0;
			BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, float(frame) / VIDEO_DUMP_FRAMERATE));
#endif

			static float last_time = 0.f;
			static float time_offset = 0.f;
			float time = synctimer.getTime();
			float beat = synctimer.getRow();
//			time = timeGetTime() / 1000.0f;

#ifndef VJSYS
			syncDevice->update(beat); //gets current timing info from the SyncTimer.
#endif
			device->BeginScene();
			
			/* setup multisampled stuffz */
			device.setRenderTarget(color_msaa.getRenderTarget());
			device.setDepthStencilSurface(depthstencil_msaa);
/*
			device.setRenderTarget(color_hdr.getRenderTarget());
			device.setDepthStencilSurface(depthstencil_hdr);
*/
			D3DXCOLOR clear_color(0.45f, 0.25f, 0.25f, 0.f);
//			D3DXCOLOR clear_color(spectrum[0] * 1.5f, 0.f, 0.f, 0.f);

			float roll = cameraRollTrack.getValue(beat);
			roll *= float(2 * M_PI);
			float yRot = cameraYRotTrack.getValue(beat);
			Vector3 up(float(sin(roll)), float(cos(roll)), 0.f);
			Vector3 eye(
				float(sin(yRot)),
				float(cameraUpTrack.getValue(beat)),
				float(cos(yRot))
			);
			eye = normalize(eye);
			
			float camera_distance = 360 * (cameraDistanceTrack.getValue(beat));
			eye *= camera_distance;
			float shake_time = time * 0.125f * (cameraShakeTempoTrack.getValue(beat));
			Vector3 at(0, 0, 0);
			
			eye = Vector3(sin(beat * 0.005f), 0, -cos(beat * 0.005f)) * 80;
			at = Vector3(20, 0, 0);
			eye -= at;
			
			at += Vector3(
				pow(sin(shake_time * 15 - cos(shake_time * 20)), 3),
				pow(cos(shake_time * 15 - sin(shake_time * 21)), 3),
				pow(cos(shake_time * 16 - sin(shake_time * 20)), 3)
				) * 0.05f * math::length(eye - at) * (cameraShakeAmtTrack.getValue(beat));
			
			Matrix4x4 world;
			world.makeIdentity();
			
			Matrix4x4 view;
			view.makeLookAt(eye, at, roll);
			Matrix4x4 proj = Matrix4x4::projection(60.0f, float(DEMO_ASPECT), 0.1f, 1000.f);
			
			
/*			testScene->anim(fmod(beat, 100));
			scenegraph::Camera *cam = testScene->findCamera("Camera01-camera");
			if (NULL != cam)
			{
				Matrix4x4 camView = cam->getAbsoluteTransform();
				camView *= Matrix4x4::rotation(math::Quaternion(
					(pow(sin(shake_time * 15 - cos(shake_time * 20)), 3) / 100) * cameraShakeAmtTrack.getValue(beat),
					(pow(cos(shake_time * 15 - sin(shake_time * 21)), 3) / 100) * cameraShakeAmtTrack.getValue(beat),
					0
					)
				);
				view = camView.inverse();
				proj = cam->getProjection();
			} */
			
			skybox_fx->setMatrices(world, view, proj);
			skybox_fx->commitChanges();
			
			voxelMesh.setSize((1.5f + sin(time / 8)) * 32);
			float grid_size = voxelMesh.getSize();
			Matrix4x4 mrot;
			mrot.makeRotation(Vector3(float(-M_PI / 2), float(M_PI - sin(time / 5)), float(M_PI + time / 3)));
			Matrix4x4 mscale;
			mscale.makeScaling(Vector3(0.75f, 0.75f, 0.75f));
/*			voxelMesh.update(mrot * mscale); */

			for (int i = 0; i < 2; ++i)
			{
				device->Clear(0, 0, D3DCLEAR_ZBUFFER, clear_color, 1.f, 0);
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
				
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
				skybox_fx->draw(cube_x);
				
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

				//	test_fx.draw(cube_x);
				//	time /= 4;

/*				{
					testRenderer.view       = view;
					testRenderer.projection = proj; // math::Matrix4x4::projection(60.0f, 16.0f / 9, 1.0f, 1000.0f);
					testRenderer.draw();
				} */
				
				for (int i = 0; i < 20; ++i)
				{
					Matrix4x4 world = Matrix4x4::rotation(math::Quaternion(M_PI / 2, M_PI / 2, 0));
//					world = Matrix4x4::rotation(math::Quaternion(0, M_PI, 0)) * world;
					world *= Matrix4x4::translation(Vector3(-i * 300, 0, 0));
					float explode = math::clamp((beat - (255 + 48)) * 0.5f, 0.0f, 1.0f);
					float scale = 0.25f * explode;
					world *= Matrix4x4::scaling(Vector3(scale, scale, scale));
					tunelle_fx->setMatrices(world, view, proj);
					tunelle_fx->commitChanges();
					tunelle_fx->draw(tunelle_x);
				}
				
#if 0
				Matrix4x4 mscale2;
				mscale2.makeScaling(Vector3(10.0f / grid_size, 10.0f / grid_size, 10.0f / grid_size));
				
				// center object
				Vector3 pos = Vector3(floor(grid_size / 2), floor(grid_size / 2), floor(grid_size / 2));
				world = math::Matrix4x4::translation(-pos);
				cubegrid_fx.setMatrices(world * mscale2, view, proj);
				
				float planearray[4];
				Matrix4x4 hatchTransform;
				
				planearray[0] = sin(time);
				planearray[1] = cos(time);
				planearray[2] = 0;
				planearray[3] = 0;
				device->SetClipPlane(0, planearray);
				hatchTransform = Matrix4x4::scaling(Vector3(
					25.0f / device.getRenderTarget().getDesc().Width,
					25.0f / device.getRenderTarget().getDesc().Width,
					1)
				);
				cubegrid_fx.setMatrix("hatch_transform", hatchTransform);
				cubegrid_fx->CommitChanges();
				
				device->SetRenderState( D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0 );
//				voxelMesh.draw(device);
				
				planearray[0] *= -1;
				planearray[1] *= -1;
				planearray[2] *= -1;
				device->SetClipPlane(0, planearray);
				
				hatchTransform = Matrix4x4::scaling(Vector3(
					15.0f / device.getRenderTarget().getDesc().Width,
					15.0f / device.getRenderTarget().getDesc().Width,
					1)
					) * Matrix4x4::rotation(Vector3(0, 0, M_PI / 2));
				cubegrid_fx.setMatrix("hatch_transform", hatchTransform);
				cubegrid_fx->CommitChanges();
				
//				voxelMesh.draw(device);
				device->SetRenderState( D3DRS_CLIPPLANEENABLE, 0 );
#endif

#if 0
				//				world = math::Matrix4x4::translation(Vector3(0, 0, 20));
				/* explosion */
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
				explosion_fx->setMatrices(world, view, proj);
				explosion.draw(*explosion_fx, explosionTrack.getIntValue(beat));
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
#endif

#if 1
				/* particles */
				{
					Matrix4x4 modelview = world * view;
//					cloud.sort(Vector3(modelview._13, modelview._23, modelview._33));
					particle_fx->setMatrices(world, view, proj);
					
					Vector3 up(modelview._12, modelview._22, modelview._32);
					Vector3 left(modelview._11, modelview._21, modelview._31);
					math::normalize(up);
					math::normalize(left);
					
					particle_fx->setFloatArray("up", up, 3);
					particle_fx->setFloatArray("left", left, 3);
					particle_fx->commitChanges();
					
					device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
					device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
					device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
					device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
//					device->SetRenderState(D3DRS_ZWRITEENABLE, false);
					
					engine::ParticleCloud<ParticleData>::ParticleContainer::iterator iter = cloud.particles.begin();
					bool iter_done = false;
					
					float explode = std::max((beat - 255) * 0.5f, 0.0f);
					explode *= explode;
					particle_fx->setFloat("overbright", std::min(explode, 1.0f));
					
					int p = 0;
					while (!iter_done)
					{
						streamer.begin();
						for (int i = 0; i < 1024; ++i)
						{
							Vector3 pos = iter->pos;
							pos = pos + pos * pow(1.0f / math::length(pos), 1.75f) * explode;
							float size = iter->data.size;
							
							streamer.add(pos, size);
							++iter;
							++p;
							
							if (cloud.particles.end() == iter)
							{
								iter_done = true;
								break;
							}
						}
						streamer.end();
						particle_fx->draw(&streamer);
					}
				}
				
				/* particles */
				{
					Matrix4x4 modelview = world * view;
					//					cloud.sort(Vector3(modelview._13, modelview._23, modelview._33));
					particle2_fx->setMatrices(world, view, proj);
					
					Vector3 up(modelview._12, modelview._22, modelview._32);
					Vector3 left(modelview._11, modelview._21, modelview._31);
					math::normalize(up);
					math::normalize(left);

					particle2_fx->setFloatArray("up", up, 3);
					particle2_fx->setFloatArray("left", left, 3);
					particle2_fx->setFloat("alpha", 0.1f);
					particle2_fx->commitChanges();
					
					device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
					device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
					device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
					device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
					device->SetRenderState(D3DRS_ZWRITEENABLE, false);
					
					engine::ParticleCloud<ParticleData>::ParticleContainer::iterator iter = cloud2.particles.begin();
					bool iter_done = false;
					
					float explode = std::max((beat - 255), 0.0f) * 0.2f;
					explode *= explode;
					
					int p = 0;
					while (!iter_done)
					{
						streamer.begin();
						for (int i = 0; i < 1024; ++i)
						{
							Vector3 pos = iter->pos;
							pos = pos + pos * pow(1.0f / math::length(pos), 1.75f) * explode;
							float size = iter->data.size;
							
							streamer.add(pos, size);
							++iter;
							++p;
							
							if (cloud2.particles.end() == iter)
							{
								iter_done = true;
								break;
							}
						}
						streamer.end();
						particle2_fx->draw(&streamer);
					}
				}
				
				D3DLOCKED_RECT rect;
				if (!FAILED(logo_surf->LockRect(&rect, NULL, D3DLOCK_READONLY)))
				{
					tex_trans_fx->setTexture("tex", bar_tex);
					tex_trans_fx->setFloat("alpha", 1.0f);
					
					float anim = std::max(beat - 64, 0.0f) / 4;
					
					float scale = 10;
					Matrix4x4 world = Matrix4x4::scaling(math::Vector3(scale, scale, scale));
					world = Matrix4x4::translation(Vector3(2, 0, 0)) * world;
					
					unsigned int *data = (unsigned int*)rect.pBits;
					for (size_t y = 0; y < logo_surf.getDesc().Height; ++y)
					{
						for (size_t x = 0; x < logo_surf.getDesc().Width; ++x)
						{
							unsigned int color = ((unsigned int*)((char*)rect.pBits + rect.Pitch * y))[x];
							if ((color & 0xFFFFFF) != 0)
							{
								tex_trans_fx->setMatrices(
									
									Matrix4x4::translation(Vector3(x * 0.1f, y * -0.1f + 3 / std::max((int(y) - 10) + anim * 4, 0.0f), 0)) *
									world *
									Matrix4x4::rotation(Vector3(cos(anim - x * 0.1) / (anim + 1), sin(anim + y * 0.15) / (anim + 1), 0)) * 
									Matrix4x4::translation(Vector3(0, 0, 4 - 4 / (anim + 1))),
									view,
									proj);
								drawQuad(
									device, tex_trans_fx,
									-0.2f, 0.1f,
									0.2f, -0.1f,
									0, 0
									);
							}
						}
					}
					logo_surf->UnlockRect();
				}
				
				device->SetRenderState(D3DRS_ZWRITEENABLE, true);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
#endif
				
				device.setRenderTarget(color1_hdr.getRenderTarget());
				device.setDepthStencilSurface(depthstencil);
			}
#if 1
			world.makeIdentity();
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
//			device->SetRenderState(D3DRS_ZWRITEENABLE, false);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

			blur_fx->setFloat("sub", 0.25f);
			RenderTexture render_textures[2] = { color1_hdr, color2_hdr };
			int rtIndex = 0;
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 2; j++)
				{
					RenderTexture current_rt  = render_textures[(rtIndex + 1) & 1];
					RenderTexture current_tex = render_textures[(rtIndex + 0) & 1];
					
					device.setRenderTarget(current_rt.getRenderTarget(), 0);
					device->SetDepthStencilSurface(NULL);
//					device->Clear(0, 0, D3DCLEAR_TARGET, clear_color, 1.f, 0);
					
					float dir_vec[2];
					float dir = float(M_PI / 2 * j);
					dir_vec[0] = cosf(dir) * (float(1 << i) / current_tex.getWidth());
					dir_vec[1] = sinf(dir) * (float(1 << i) / current_tex.getWidth());
					dir_vec[0] *= 3.f / 4.f;
					
					blur_fx->setFloatArray("dir", dir_vec, 2);
					blur_fx->setTexture("blur_tex", current_tex);
					
					drawQuad(
						device, blur_fx,
						-1.0f, -1.0f,
						 2.0f, 2.0f
					);
					blur_fx->setFloat("sub", 0.0f);
					rtIndex++;
				}
			}
#endif
			
			/* letterbox */
			device.setRenderTarget(backbuffer);
			device->SetDepthStencilSurface(NULL);
			device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.f, 0);
			device.setViewport(&letterbox_viewport);
			
			color_msaa.resolve(device);
			
			color_map_fx->setFloat("fade", colorMapBlendTrack.getValue(beat));
			color_map_fx->setFloat("flash", pow(colorMapFlashTrack.getValue(beat), 2.0f));
			color_map_fx->setFloat("fade2", colorMapFadeTrack.getValue(beat));
			color_map_fx->setFloat("alpha", 0.25f);
			color_map_fx->setTexture("tex", color1_hdr);
			color_map_fx->setTexture("tex2", color_msaa);
//			color_map_fx->setTexture("color_map", color_maps[colorMapPalTrack.getIntValue(beat) % 2]);
			color_map_fx->setTexture("color_map", color_maps[0]);
			color_map_fx->setTexture("desaturate", desaturate_tex);
			
			device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
			drawFuzz(color_map_fx, vertex_streamer,  time, 1.0f, colorMapDistortXTrack.getValue(beat), colorMapDistortYTrack.getValue(beat),
				0.5f / letterbox_viewport.Width, 0.5f / letterbox_viewport.Height);
			device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED);
			drawFuzz(color_map_fx, vertex_streamer, -time, 0.0f, colorMapDistortXTrack.getValue(beat), 0.0f,
				0.5f / letterbox_viewport.Width, 0.5f / letterbox_viewport.Height);
			device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
			
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
			
			/* draw noise */
			
			noise_fx->setFloat("alpha", (noiseAmtTrack.getValue(beat) + noiseFFTTrack.getValue(beat) * noise_fft.getValue(beat)));
			noise_fx->setTexture("tex", noise_tex);
			drawQuad(
				device, noise_fx,
				-1.0f, -1.0f,
				 2.0f, 2.0f,
				 randf(), randf()
			);
			
			/* draw scanlines */
			scanlinesImage.setPosition(-1, -1);
			scanlinesImage.setDimension(2, 2);
			scanlinesImage.draw(device);
			
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			
			device->EndScene(); /* WE DONE IS! */
			
#ifdef DUMP_VIDEO
			{
				char temp[256];
				_snprintf(temp, 256, "dump/frame%04d.tga", frame);
				core::d3dErr(D3DXSaveSurfaceToFile(
					temp,
					D3DXIFF_TGA,
					backbuffer,
					NULL,
					NULL
				));
				frame++;
			}
#endif
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
