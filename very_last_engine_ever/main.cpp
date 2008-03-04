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

#include "scenegraph/scene.h"
#include "scenegraph/prstransform.h"
#include "scenegraph/meshnode.h"
#include "scenegraph/target.h"
#include "tinyxml.h"

using scenegraph::Node;
using scenegraph::Scene;
using scenegraph::PrsTransform;
using scenegraph::MeshNode;

static void loadChildren(renderer::Device &device, Node *graphNode, TiXmlElement *xmlElem);

static float loadFloat(TiXmlElement *elem, const char *name, float default_val)
{
	const char *ret = elem->Attribute(name);
	if (!ret) return default_val;
	return float(atof(ret));
}

static std::string loadString(TiXmlElement *elem, const char *name, const std::string &defaultString = "")
{
	const char *ret = elem->Attribute(name);
	if (NULL == ret) return defaultString;
	return ret;
}

static math::Vector3 loadVector3(TiXmlNode* node)
{
	TiXmlElement *elem = node->ToElement();
	assert(NULL != elem);
	
	return Vector3(
		loadFloat(elem, "x", 0),
		loadFloat(elem, "y", 0),
		loadFloat(elem, "z", 0)
	);
}

static Node *loadPrsTransform(renderer::Device &device, TiXmlElement *xmlElem)
{
	PrsTransform *ret = new PrsTransform(loadString(xmlElem, "name"));
	
	ret->setPosition(Vector3(0, 0, 0));
	ret->setRotation(Vector3(0, 0, 0));
	ret->setScale(Vector3(1, 1, 1));
	
	TiXmlNode* curr = xmlElem->FirstChild();
	while (curr)
	{
		if (curr->Type() == TiXmlNode::ELEMENT)
		{
			TiXmlElement *currElem = curr->ToElement();
			assert(NULL != currElem);
			
			if      (strcmp(curr->Value(), "position") == 0) ret->setPosition(loadVector3(currElem));
			else if (strcmp(curr->Value(), "children") == 0) loadChildren(device, ret, currElem);
			else throw std::string("unknown element \"") + curr->Value() + std::string("\"");
		}
		curr = xmlElem->IterateChildren(curr);
	}
	
	return ret;
}

static Node *loadMesh(renderer::Device &device, TiXmlElement *xmlElem)
{
	std::string fileName = loadString(xmlElem, "file");
	engine::loadMesh(device, fileName);
	
	MeshNode *ret = new scenegraph::MeshNode(loadString(xmlElem, "name"), NULL, NULL);
	return ret;
}

static Node *loadTarget(TiXmlElement *xmlElem)
{
	scenegraph::Target *ret = new scenegraph::Target(loadString(xmlElem, "name"));
	ret->setTarget(loadVector3(xmlElem));
	
	return ret;
}

static void loadChildren(renderer::Device &device, Node *graphNode, TiXmlElement *xmlElem)
{
	TiXmlNode* curr = xmlElem->FirstChild();
	while (curr)
	{
		if (curr->Type() == TiXmlNode::ELEMENT)
		{
			TiXmlElement *currElem = curr->ToElement();
			assert(NULL != currElem);
			
			Node *newChild = NULL;
			if      (strcmp(curr->Value(), "prs_transform") == 0) newChild = loadPrsTransform(device, currElem);
			else if (strcmp(curr->Value(), "mesh") == 0)          newChild = loadMesh(device, currElem);
			else if (strcmp(curr->Value(), "target") == 0)        newChild = loadTarget(currElem);
			else throw std::string("unknown element \"") + curr->Value() + std::string("\"");
			
			assert(NULL != newChild);
			graphNode->addChild(newChild);
		}
		curr = xmlElem->IterateChildren(curr);
	}
}

Scene *loadScene(renderer::Device &device, const std::string filename)
{
	Scene *scene = new Scene(filename);
	TiXmlDocument doc;
	try
	{
		if (!doc.LoadFile(filename.c_str()))
		{
			if (doc.ErrorRow() > 0)
			{
				char temp[256];
				_snprintf(temp, 256, "error at line %d: %s", doc.ErrorRow(), doc.ErrorDesc());
				throw std::string(temp);
			}
			else throw std::string(doc.ErrorDesc());
		}
		
		TiXmlElement *xmlRoot = doc.RootElement();
		if (strcmp(xmlRoot->Value(), "scene") != 0) throw std::string("invalid root node \"") + xmlRoot->Value() + std::string("\"");
		
		TiXmlNode* curr = xmlRoot->FirstChild();
		while (curr)
		{
			if (curr->Type() == TiXmlNode::ELEMENT)
			{
				TiXmlElement *currElem = curr->ToElement();
				assert(NULL != currElem);
				
				if      (strcmp(curr->Value(), "children") == 0) loadChildren(device, scene, currElem);
				else throw std::string("unknown element \"") + curr->Value() + std::string("\"");
			}
			curr = xmlRoot->IterateChildren(curr);
		}
		
		return scene;
	}
	catch (const std::string &str)
	{
		delete scene;
		throw core::FatalException("Failed to load scene \"" + filename + "\":\n"+ str);
	}
}

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
/*
		{
			MessageBox(NULL, "Selected mode does not support FP16 blending, demo will look crap.", "visual quality warning", MB_OK | MB_ICONWARNING);
		}
*/
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
//		if (!BASS_Init(config.getSoundcard(), 44100, BASS_DEVICE_LATENCY, 0, 0)) throw FatalException("failed to init bass");
		if (!BASS_Init(config.getSoundcard(), 44100, 0, 0, 0)) throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/glitch.ogg", 0, 0, BASS_MP3_SETPOS | ((0 == config.getSoundcard()) ? BASS_STREAM_DECODE : 0));
		if (!stream) throw FatalException("failed to open tune");
		
		SyncTimerBASS_Stream synctimer(stream, BPM, 4);
		
#ifdef SYNC
		SyncEditor sync("data\\__data_%s_%s.sync", synctimer);
#else
		Sync sync("data\\__data_%s_%s.sync", synctimer);
#endif
		SyncTrack &cameraDistanceTrack = sync.getTrack("distance", "camera", 6, false);
		SyncTrack &cameraRollTrack     = sync.getTrack("roll", "camera", 6, true);
		SyncTrack &cameraYRotTrack     = sync.getTrack("y-rot", "camera", 6, true);
		SyncTrack &cameraUpTrack     = sync.getTrack("up", "camera", 6, true);
		SyncTrack &cameraShakeAmtTrack     = sync.getTrack("shake amt", "camera", 8, true);
		SyncTrack &cameraShakeTempoTrack     = sync.getTrack("shake tempo", "camera", 6, true);

		SyncTrack &colorMapBlendTrack  = sync.getTrack("blend", "color map", 6, true);
		SyncTrack &colorMapPalTrack    = sync.getTrack("pal",   "color map", 4,   true);
		SyncTrack &colorMapFadeTrack   = sync.getTrack("fade",  "color map", 4,   true);
		SyncTrack &colorMapFlashTrack  = sync.getTrack("flash", "color map", 4,   true);

		SyncTrack &noiseAmtTrack  = sync.getTrack("amt", "noise", 4,   true);
		SyncTrack &noiseFFTTrack  = sync.getTrack("fft", "noise", 4,   true);

		SyncTrack &jellyAmtX = sync.getTrack("amt_x", "jelly", 6, true);
		SyncTrack &jellyAmtY = sync.getTrack("amt_y", "jelly", 6, true);
		SyncTrack &jellyAmtZ = sync.getTrack("amt_z", "jelly", 6, true);

		SyncTrack &jellyScaleX = sync.getTrack("scale_x", "jelly", 6, true);
		SyncTrack &jellyScaleY = sync.getTrack("scale_y", "jelly", 6, true);
		SyncTrack &jellyScaleZ = sync.getTrack("scale_z", "jelly", 6, true);

		SyncTrack &jellySwimTrack = sync.getTrack("swim", "jelly", 6, true);

		SyncTrack &textAlpha1Track = sync.getTrack("alpha1", "text", 6, true);
		SyncTrack &textAlpha2Track = sync.getTrack("alpha2", "text", 6, true);
		SyncTrack &textBlink1Track = sync.getTrack("blink1", "text", 6, true);
		SyncTrack &textBlink2Track = sync.getTrack("blink2", "text", 6, true);

		engine::SpectrumData noise_fft = engine::loadSpectrumData("data/noise.fft");

		Surface backbuffer   = device.getRenderTarget(0);
		Surface depthstencil = device.getDepthStencilSurface();
		
		RenderTexture color_msaa(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8, config.getMultisample());
		Surface depthstencil_msaa = device.createDepthStencilSurface(letterbox_viewport.Width, letterbox_viewport.Height, D3DFMT_D24S8, config.getMultisample());

		/** DEMO ***/

		RenderTexture color1_hdr(device, 800 / 2, int((800 / DEMO_ASPECT) / 2), 1, D3DFMT_A16B16G16R16F);
		RenderTexture color2_hdr(device, 800 / 2, int((800 / DEMO_ASPECT) / 2), 1, D3DFMT_A16B16G16R16F);

		scenegraph::Scene *testScene = loadScene(device, "data/test.scene");

//		RenderTexture rt(device, 128, 128, 1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE);
//		RenderTexture rt2(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8);
//		RenderTexture rt3(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8);

		Matrix4x4 tex_transform;
		tex_transform.make_identity();
		Effect tex_fx      = engine::loadEffect(device, "data/tex.fx");
		tex_fx->SetMatrix("transform", &tex_transform);
		Effect blur_fx     = engine::loadEffect(device, "data/blur.fx");

		Effect particle_fx = engine::loadEffect(device, "data/particle.fx");
		Texture bartikkel_tex = engine::loadTexture(device, "data/particle.png");
		particle_fx->SetTexture("tex", bartikkel_tex);

		Effect noise_fx = engine::loadEffect(device, "data/noise.fx");
		Texture noise_tex = engine::loadTexture(device, "data/noise.png");

		Effect color_map_fx = engine::loadEffect(device, "data/color_map.fx");
		Image color_image(color1_hdr, color_map_fx);
//		Image color_image(color_msaa, color_map_fx);
		Texture color_maps[2];
		
		color_maps[0] = engine::loadTexture(device, "data/color_map0.png");
		color_maps[1] = engine::loadTexture(device, "data/color_map1.png");
		color_map_fx->SetFloat("texel_width", 1.0f / color_msaa.getWidth());
		color_map_fx->SetFloat("texel_height", 1.0f / color_msaa.getHeight());

		Effect cubegrid_fx = engine::loadEffect(device, "data/cubegrid.fx");
		renderer::VolumeTexture front_tex = engine::loadVolumeTexture(device, "data/front.dds");
		cubegrid_fx->SetTexture("front_tex", front_tex);

		engine::VoxelGrid voxelGrid = engine::loadVoxelGrid("data/duck.voxel");
		engine::VoxelMesh voxelMesh(device, cubegrid_fx, voxelGrid, 64);
		
		engine::ParticleStreamer streamer(device);
		struct ParticleData
		{
			ParticleData(float size, const Vector3 &dir) : size(size), dir(dir) {}
			float size;
			Vector3 dir;
		};
		engine::ParticleCloud<ParticleData> cloud;
		for (int i = 0; i < 1024 * 4; ++i)
		{
/*			float x = (randf() - 0.5f) * 185;
			float z = (randf() - 0.5f) * 185;
			float y = (randf() - 0.5f) * 150;
			float s = (randf() + 0.75f) * 0.5f;
			cloud.addParticle(engine::Particle<float>(Vector3(x, y, z), s)); */
		}
		for (int i = 0; i < 1024 * 4; ++i)
		{
			float x = (randf() - 0.5f);
			float z = (randf() - 0.5f);
			float y = (randf() - 0.5f);
			float s = (randf() + 0.75f) * 0.55f;
			cloud.addParticle(
				engine::Particle<ParticleData>(
					Vector3(x, y, z) * 100,
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


		Image scanlinesImage(engine::loadTexture(device, "data/scanlines.png"), tex_fx);

		renderer::CubeTexture cubemap_tex = engine::loadCubeTexture(device, "data/stpeters_cross3.dds");
/*
		Effect test_fx = engine::loadEffect(device, "data/test.fx");
		test_fx->SetTexture("env", cube);
*/
		Effect jellyfish_fx = engine::loadEffect(device, "data/jellyfish.fx");
		jellyfish_fx->SetTexture("reflectionMap", cubemap_tex);
		Mesh sphere_x       = engine::loadMesh(device, "data/sphere2.x");

		Effect skybox_fx = engine::loadEffect(device, "data/skybox.fx");
		skybox_fx->SetTexture("reflectionMap", cubemap_tex);
		Mesh cube_x         = engine::loadMesh(device, "data/cube.X");

		Effect text_fx              = engine::loadEffect(device, "data/text.fx");
		Texture excess_outline_tex = engine::loadTexture(device, "data/excess_outline.dds");
		Texture demotitle_tex      = engine::loadTexture(device, "data/demotitle.dds");

		BASS_Start();
		BASS_ChannelPlay(stream, false);
		BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, 20.0f));


#ifdef SYNC
		sync.showEditor();
#endif

		bool done = false;
		while (!done)
		{
#ifndef VJSYS
			if (!sync.doEvents()) done = true;
#endif

#ifdef DUMP_VIDEO
			static int frame = 0;
			BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, float(frame) / VIDEO_DUMP_FRAMERATE));
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
			device.setRenderTarget(color_hdr.getRenderTarget());
			device.setDepthStencilSurface(depthstencil_hdr);
*/
			D3DXCOLOR clear_color(0.45f, 0.25f, 0.25f, 0.f);
//			D3DXCOLOR clear_color(spectrum[0] * 1.5f, 0.f, 0.f, 0.f);

			float roll = cameraRollTrack.getFloatValue() / 256;
			roll *= float(2 * M_PI);
			float yRot = cameraYRotTrack.getFloatValue() / 256;
			Vector3 up(float(sin(roll)), float(cos(roll)), 0.f);
			Vector3 eye(
				float(sin(yRot)),
				float(cameraUpTrack.getFloatValue() / 256),
				float(cos(yRot))
			);
			eye = normalize(eye);

			float camera_distance = 60 * (cameraDistanceTrack.getFloatValue() / 256);
			eye *= camera_distance;
			float shake_time = time * 0.125f * (cameraShakeTempoTrack.getFloatValue() / 256);
			Vector3 at(0, 0, 0);
			at += Vector3(
				pow(sin(shake_time * 15 - cos(shake_time * 20)), 3),
				pow(cos(shake_time * 15 - sin(shake_time * 21)), 3),
				pow(cos(shake_time * 16 - sin(shake_time * 20)), 3)
			) * 0.05f * camera_distance * (cameraShakeAmtTrack.getFloatValue() / 256);

			Matrix4x4 world;
			world.make_identity();

			Matrix4x4 view;
			view.makeLookAt(eye + at, at, roll);
			Matrix4x4 proj;
			proj.make_projection(60.0f, float(DEMO_ASPECT), 0.1f, 1000.f);
			
			jellyfish_fx.setVector3("amt", Vector3(
				jellyAmtX.getFloatValue() / 256,
				jellyAmtY.getFloatValue() / 256,
				jellyAmtZ.getFloatValue() / 256)
			);
			jellyfish_fx.setVector3("scale", Vector3(
				jellyScaleX.getFloatValue() / 256,
				jellyScaleY.getFloatValue() / 256,
				jellyScaleZ.getFloatValue() / 256)
			);
			jellyfish_fx.setVector3("phase", Vector3(
				time,
				time * 2,
				time * 10)
			);
			jellyfish_fx->SetFloat("time", time);
			jellyfish_fx.setVector3("vViewPosition", eye + at);
			jellyfish_fx.setMatrices(world, view, proj);

			skybox_fx.setMatrices(world, view, proj);

			
			int blink1_val = textBlink1Track.getIntValue();
			float blink1 = 1.0f;
			if (blink1_val != 0) blink1 = math::frac(time * blink1_val) > 0.5f ? 1.0f : 0.0f;

			int blink2_val = textBlink2Track.getIntValue();
			float blink2 = 1.0f;
			if (blink2_val != 0) blink2 = math::frac(time * blink2_val) > 0.5f ? 1.0f : 0.0f;
			
			text_fx->SetFloat("alpha1", (textAlpha1Track.getFloatValue() / 256) * blink1);
			text_fx->SetFloat("alpha2", (textAlpha2Track.getFloatValue() / 256) * blink2);
			text_fx->SetTexture("excess", excess_outline_tex);
			text_fx->SetTexture("demotitle", demotitle_tex);
			text_fx.setMatrices(world, view, proj);
			
			for (int i = 0; i < 2; ++i)
			{
				device->Clear(0, 0, D3DCLEAR_ZBUFFER, clear_color, 1.f, 0);
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
				

//				jellyfish_fx.draw(sphere_x);

				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
				skybox_fx.draw(cube_x);


//				device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
				device->SetRenderState(D3DRS_ZWRITEENABLE, false);
				engine::drawQuad(device, text_fx, -45, -45, 90, 90);
				device->SetRenderState(D3DRS_ZWRITEENABLE, true);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);



				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

				//			test_fx.draw(cube_x);

				//			time /= 4;

				Matrix4x4 mrot;
				mrot.make_identity();
				mrot.make_rotation(Vector3(0, time, 0));
				mrot.make_rotation(Vector3(float(-M_PI / 2), float(M_PI - sin(time / 5)), float(M_PI + time / 3)));
				Matrix4x4 mscale;
				float scale = 0.75f;
				mscale.make_scaling(Vector3(scale, scale, scale));

				Matrix4x4 mscale2;
				mscale2.make_scaling(Vector3(5, 5, 5));
				world.make_translation(Vector3(-5, -5, -5));
				cubegrid_fx.setMatrices(world * mscale2, view, proj);
				voxelMesh.setSize((1.5f + sin(time / 8)) * 32);
				voxelMesh.update(mrot * mscale);
				voxelMesh.draw(device);


#if 1
				/* particles */
				float particleScroll = 0.0f; // -jellySwimTrack.getFloatValue() / (10 * 256);
				for (int i = 0; i < 1; ++i)
				{
					world.make_translation(Vector3(0, (math::frac(particleScroll) - i) * 150, 0));
					Matrix4x4 modelview = world * view;
					cloud.sort(Vector3(modelview._13, modelview._23, modelview._33));
					
					particle_fx.setMatrices(world, view, proj);
					
					{
						Vector3 up(modelview._12, modelview._22, modelview._32);
						Vector3 left(modelview._11, modelview._21, modelview._31);
						math::normalize(up);
						math::normalize(left);
						
						particle_fx->SetFloatArray("up", up, 3);
						particle_fx->SetFloatArray("left", left, 3);
						particle_fx->SetFloat("alpha", 0.1f);
						particle_fx->CommitChanges();
					}

					device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
					device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
					device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
					device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
					device->SetRenderState(D3DRS_ZWRITEENABLE, false);
					
					engine::ParticleCloud<ParticleData>::ParticleContainer::iterator iter = cloud.particles.begin();
					bool iter_done = false;
					
					int p = 0;
					while (!iter_done)
					{
						streamer.begin();
						for (int i = 0; i < 1024; ++i)
						{
							streamer.add(
								Vector3(iter->pos.x,
										iter->pos.y,
										iter->pos.z
								),
								iter->data.size // + (1 - fmod(beat, 1.0)) * 2
							);
	//						iter->pos += iter->data.dir * 0.1f;
	//						if (math::length(iter->pos) > 100.0f) iter->pos = Vector3(0,0,0);
							float theta = float(p) * 0.005f;
							float thetaa = theta - time;
							float radius = 50.0f;
							iter->pos = Vector3(cos(thetaa) * 3.1, cos(thetaa) * 2, sin(thetaa) * 3);
							iter->pos = Vector3(cos(iter->pos.x - iter->pos.y * 0.93f), cos(iter->pos.y * 1.5f), sin(iter->pos.z - iter->pos.x));
							iter->pos = normalize(iter->pos) * radius;
							iter->data.size = 10.0f / (1 + theta);
							
							++iter;
							++p;

							if (cloud.particles.end() == iter)
							{
								iter_done = true;
								break;
							}
						}
						streamer.end();
						particle_fx.draw(streamer);
					}
				}
				device->SetRenderState(D3DRS_ZWRITEENABLE, true);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
#endif

				device.setRenderTarget(color1_hdr.getRenderTarget());
				device.setDepthStencilSurface(depthstencil);
			}
#if 1
			world.make_identity();
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
//			device->SetRenderState(D3DRS_ZWRITEENABLE, false);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

			blur_fx->SetFloat("sub", 0.5f);
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

					blur_fx->SetFloatArray("dir", dir_vec, 2);
					blur_fx->SetTexture("blur_tex", current_tex);

					drawQuad(
						device, blur_fx,
						-1.0f, -1.0f,
						 2.0f, 2.0f
					);
					blur_fx->SetFloat("sub", 0.0f);
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
			
			color_map_fx->SetFloat("fade", colorMapBlendTrack.getFloatValue() * (1.f / 256));
			color_map_fx->SetFloat("flash", pow(colorMapFlashTrack.getFloatValue() / 256, 2.0f));
			color_map_fx->SetFloat("fade2", colorMapFadeTrack.getFloatValue() / 256);
			color_map_fx->SetFloat("alpha", 0.25f);
			color_map_fx->SetTexture("tex2", color_msaa);
			color_map_fx->SetTexture("color_map", color_maps[colorMapPalTrack.getIntValue() % 2]);

			color_image.setPosition(-1, -1);
			color_image.setDimension(2, 2);
			color_image.draw(device);

			scanlinesImage.setPosition(-1, -1);
			scanlinesImage.setDimension(2, 2);

			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);

			/* draw noise */
			
			noise_fx->SetFloat("alpha", (noiseAmtTrack.getFloatValue() + noiseFFTTrack.getFloatValue() * noise_fft.getValue(time)) / 256 );
			noise_fx->SetTexture("tex", noise_tex);
			drawQuad(
				device, noise_fx,
				-1.0f, -1.0f,
				 2.0f, 2.0f,
				 randf(), randf()
			);

			/* draw scanlines */
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
