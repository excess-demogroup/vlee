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
#define OSC_SIZE 512
float osc[OSC_SIZE];

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

void drawOsc(Effect *effect, engine::VertexStreamer &streamer)
{
	UINT passes;
	(*effect)->Begin(&passes, 0);
	for (UINT pass = 0; pass < passes; ++pass) {
		(*effect)->BeginPass( pass );
		streamer.begin(D3DPT_LINESTRIP);
		float last_xoffs = 0.f;
		float last_yoffs = 0.f;
		for (unsigned i = 0; i < OSC_SIZE; ++i) {
			float x = (float(i) / (OSC_SIZE - 1)) * 2 - 1;
			float y = osc[i];
			streamer.vertex(D3DXVECTOR3(x, y, 0));
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

struct ParticleData
{
	ParticleData(float size, const Vector3 &dir) : size(size), dir(dir) {}
	float size;
	Vector3 dir;
};
using engine::ParticleCloud;

void drawParticleExplosion(renderer::Device &device, engine::ParticleStreamer &streamer, Effect *particle_fx, ParticleCloud<ParticleData> &blackCloud, float explode, const Matrix4x4 &modelview, float exp = 1.75f)
{
	Vector3 up(modelview._12, modelview._22, modelview._32);
	Vector3 left(modelview._11, modelview._21, modelview._31);
	math::normalize(up);
	math::normalize(left);
	
	particle_fx->setFloat("overbright", std::min(explode, 1.0f));
	particle_fx->setFloatArray("up", up, 3);
	particle_fx->setFloatArray("left", left, 3);
	particle_fx->commitChanges();
	
	engine::ParticleCloud<ParticleData>::ParticleContainer::iterator iter = blackCloud.particles.begin();
	bool iter_done = false;

	while (!iter_done)
	{
		streamer.begin();
		for (int i = 0; i < 1024; ++i)
		{
			Vector3 pos = iter->pos;
			pos = pos + pos * pow(1.0f / math::length(pos), exp) * explode;
			float size = iter->data.size;

			streamer.add(pos, size);
			++iter;

			if (blackCloud.particles.end() == iter)
			{
				iter_done = true;
				break;
			}
		}
		streamer.end();
		particle_fx->draw(&streamer);
	}
}

float faceLight[6] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

int getFace(const Vector3 &v)
{
	float saxis[3] = { v.x,  v.y,  v.z };
	float axis[3] = { abs(v.x), abs(v.y), abs(v.z) };
	int maxAxis;
	if (axis[0]>axis[1] && axis[0]>axis[2]) maxAxis=0;
	else if (axis[1] > axis[2]) maxAxis=1;
	else maxAxis=2;
	if (saxis[maxAxis]<0) maxAxis+=3;	
	return maxAxis;
}


float getCubeLightBrightness(Vector3 pos)
{
	pos = normalize(pos);
	
	float max = fabs(pos.x);
	max = std::max(max, fabs(pos.y));
	max = std::max(max, fabs(pos.z));
	return max * faceLight[getFace(pos)];
}

void drawParticleField(renderer::Device &device, engine::ParticleStreamer &streamer, Effect *particle_fx, ParticleCloud<ParticleData> &blackCloud, const Matrix4x4 &modelview)
{
	Vector3 up(modelview._12, modelview._22, modelview._32);
	Vector3 left(modelview._11, modelview._21, modelview._31);
	math::normalize(up);
	math::normalize(left);

	particle_fx->setFloatArray("up", up, 3);
	particle_fx->setFloatArray("left", left, 3);
	particle_fx->commitChanges();

	engine::ParticleCloud<ParticleData>::ParticleContainer::iterator iter = blackCloud.particles.begin();
	bool iter_done = false;

	while (!iter_done)
	{
		streamer.begin();
		for (int i = 0; i < 1024; ++i)
		{
			Vector3 pos = iter->pos;
			float size = iter->data.size;

			streamer.add(pos, size);
			++iter;

			if (blackCloud.particles.end() == iter)
			{
				iter_done = true;
				break;
			}
		}
		streamer.end();
		particle_fx->draw(&streamer);
	}
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
	
	try
	{
		/* create d3d object */
		CComPtr<IDirect3D9> direct3d = com_ptr(Direct3DCreate9(D3D_SDK_VERSION));
		if (!direct3d) throw FatalException("your directx-version is from the stone-age.\n\nTHRUG SAYS: UPGRADE!");

		/* show config dialog */
		INT_PTR result = config::showDialog(hInstance, direct3d);
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

		if (FAILED(direct3d->CheckDeviceFormat(config::adapter, DEVTYPE, config::mode.Format, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)))
		{
			MessageBox(NULL, "Selected mode does not support FP16 texture-filtering, demo will look crap.", "visual quality warning", MB_OK | MB_ICONWARNING);
		}
		
		if (FAILED(direct3d->CheckDeviceFormat(config::adapter, DEVTYPE, config::mode.Format, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)))
		{
			MessageBox(NULL, "Selected mode does not support FP16 blending, demo will look crap.", "visual quality warning", MB_OK | MB_ICONWARNING);
		}
		
		/* create window */
		win = CreateWindow("static", "very last engine ever", WS_POPUP, 0, 0, config::mode.Width, config::mode.Height, 0, 0, GetModuleHandle(0), 0);
		if (!win) throw FatalException("CreateWindow() failed. something is VERY spooky.");
		
		/* create device */
		Device device;
		device.Attach(init::initD3D(direct3d, win, config::mode, D3DMULTISAMPLE_NONE, config::adapter, config::vsync));
		
		/* showing window after initing d3d in order to be able to see warnings during init */
		ShowWindow(win, TRUE);
#if !WINDOWED
		ShowCursor(0);
#endif
		
		/* setup letterbox */
		D3DVIEWPORT9 letterbox_viewport = device.getViewport();
		makeLetterboxViewport(&letterbox_viewport, config::mode.Width, config::mode.Height, config::aspect, float(DEMO_ASPECT));
		
		/* setup sound-playback */
		if (!BASS_Init(config::soundcard, 44100, 0, 0, 0)) throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/carl_den_tredje.mp3", 0, 0, BASS_MP3_SETPOS | ((0 == config::soundcard) ? BASS_STREAM_DECODE : 0));
		if (!stream) throw FatalException("failed to open tune");
		
		// setup timer and construct sync-device
		BassTimer synctimer(stream, BPM, 16);
		
		std::auto_ptr<sync::Device> syncDevice = std::auto_ptr<sync::Device>(sync::createDevice("data/sync", synctimer));
		if (NULL == syncDevice.get()) throw FatalException("something went wrong - failed to connect to host?");
		
		Track &cameraDistanceTrack = syncDevice->getTrack("cam.dist");
		Track &cameraRollTrack     = syncDevice->getTrack("cam.roll");
		Track &cameraXRotTrack     = syncDevice->getTrack("cam.x-rot");
		Track &cameraYRotTrack     = syncDevice->getTrack("cam.y-rot");
		Track &cameraZRotTrack     = syncDevice->getTrack("cam.z-rot");
		Track &cameraUpTrack     = syncDevice->getTrack("cam.up");
		Track &cameraShakeAmtTrack     = syncDevice->getTrack("cam.shake.amt");
		Track &cameraShakeTempoTrack     = syncDevice->getTrack("cam.shake.tempo");
		Track &cameraFovTrack     = syncDevice->getTrack("cam.fov");

		Track &colorMapBlendTrack  = syncDevice->getTrack("cm.blend");
		Track &colorMapPalTrack    = syncDevice->getTrack("cm.pal");
		Track &colorMapFadeTrack   = syncDevice->getTrack("cm.fade");
		Track &colorMapFlashTrack  = syncDevice->getTrack("cm.flash");
		Track &colorMapDistortXTrack  = syncDevice->getTrack("cm.dist.x");
		Track &colorMapDistortYTrack  = syncDevice->getTrack("cm.dist.y");
		Track &overlayTrack  = syncDevice->getTrack("cm.overlay");
		Track &invmapTrack  = syncDevice->getTrack("cm.invmap");

		Track &noiseAmtTrack  = syncDevice->getTrack("noise.amt");
		Track &noiseFFTTrack  = syncDevice->getTrack("noise.fft");

		Track &greetGroupTrack  = syncDevice->getTrack("greet.group");
		Track &beatTrack = syncDevice->getTrack("beat.image");

		Track &sphereOffsetTrack = syncDevice->getTrack("sphere.offs");
		Track &partTrack = syncDevice->getTrack("_part");
		Track &repeatTrack = syncDevice->getTrack("cm.repeat");

		Track &musSize1Track = syncDevice->getTrack("mus.size1");
		Track &musBoom1Track = syncDevice->getTrack("mus.boom1");
		Track &musSize2Track = syncDevice->getTrack("mus.size2");
		Track &musBoom2Track = syncDevice->getTrack("mus.boom2");
		Track &musGroove1Track = syncDevice->getTrack("mus.groove1");
		Track &musGroove2Track = syncDevice->getTrack("mus.groove2");

		engine::SpectrumData noise_fft = engine::loadSpectrumData("data/noise.fft");
		
		Surface backbuffer   = device.getRenderTarget(0);
		Surface depthstencil = device.getDepthStencilSurface();
		
		RenderTexture color_msaa(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8, config::multisample);
		Surface depthstencil_msaa = device.createDepthStencilSurface(letterbox_viewport.Width, letterbox_viewport.Height, D3DFMT_D24S8, config::multisample);
		
		/** DEMO ***/
		
		RenderTexture color1_hdr(device, 800 / 2, int((800 / DEMO_ASPECT) / 2), 1, D3DFMT_A16B16G16R16F);
		RenderTexture color2_hdr(device, 800 / 2, int((800 / DEMO_ASPECT) / 2), 1, D3DFMT_A16B16G16R16F);
		
/*		RenderTexture depth(device, 800 / 2, int((800 / DEMO_ASPECT) / 2), 1, D3DFMT_R32F); */
		
		engine::VertexStreamer vertex_streamer(device);
		
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
		
		Effect *particle3_fx = engine::loadEffect(device, "data/particle3.fx");
		Texture particle_tex = engine::loadTexture(device, "data/particle.png");
		particle3_fx->setTexture("tex", particle_tex);
		
		Effect *noise_fx = engine::loadEffect(device, "data/noise.fx");
		Texture noise_tex = engine::loadTexture(device, "data/noise.png");
		
		Effect *color_map_fx = engine::loadEffect(device, "data/color_map.fx");
		Texture color_maps[2];
		
		color_maps[0] = engine::loadTexture(device, "data/color_map0.png");
		color_maps[1] = engine::loadTexture(device, "data/color_map1.png");
		color_map_fx->setFloat("texel_width", 1.0f / color_msaa.getWidth());
		color_map_fx->setFloat("texel_height", 1.0f / color_msaa.getHeight());
		
		Effect *cubegrid_fx = engine::loadEffect(device, "data/cubegrid.fx");
		renderer::VolumeTexture front_tex = engine::loadVolumeTexture(device, "data/front.dds");
		cubegrid_fx->setTexture("front_tex", front_tex);
		
		engine::VoxelGrid voxelGrid = engine::loadVoxelGrid("data/spikeball.voxel");
		engine::VoxelMesh voxelMesh(device, cubegrid_fx, voxelGrid, 64);
		voxelMesh.update(Matrix4x4::identity());
		
		engine::ParticleStreamer streamer(device);
		
		engine::ParticleCloud<ParticleData> blackCloud;
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
			blackCloud.addParticle(
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
		
		engine::ParticleCloud<ParticleData> whiteCloud;
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
			whiteCloud.addParticle(
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
		
		engine::ParticleCloud<ParticleData> cubeExplosionParticles;
		for (int i = 0; i < 512; ++i)
		{
			float x, y, z;
			do {
				x = (randf() - 0.5f) * 2;
				z = (randf() - 0.5f) * 2;
				y = (randf() - 0.5f) * 2;
			} while ((x * x + y * y + z * z) > 1.0f || (fabs(x) < 1e-5 && fabs(y) < 1e-5 && fabs(z) < 1e-5));

			float dist = randf();
			dist = pow(dist, 1.25f);
			dist += 0.01f;
			x *= dist;
			y *= dist;
			z *= dist;

/*			x += 35;
			y -= 35;
			z -= 35; */

			float s = (randf() + 0.25f) * 5.0f;
			cubeExplosionParticles.addParticle(
				engine::Particle<ParticleData>(
				Vector3(x, y, z) * 15,
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
		
		engine::ParticleCloud<ParticleData> korridorParticles;
		for (int i = 0; i < 24 * 1024; ++i)
		{
			float x, y, z;
			x = (randf() - 0.5f) * 2;
			z = (randf() - 0.5f) * 2;
			y = (randf() - 0.5f) * 2;
			
			float s = (randf() + 0.55f);
			korridorParticles.addParticle(
				engine::Particle<ParticleData>(
				Vector3(x, y, z) * 70,
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
		//engine::Explosion explosion = engine::Explosion(device, Vector3(32.f, -16.f, 0.f), Vector3(42.f, -26.f, -10.f));

		engine::Explosion explosion = engine::Explosion(device, Vector3(22.f, -6.f, 10.f), Vector3(32.f, -21.f, -10.f));

		Effect *ccbs_fx = engine::loadEffect(device, "data/ccbs.fx");
		Texture ccbs_tex = engine::loadTexture(device, "data/particle.png");
		ccbs_fx->setTexture("ccbs_tex", ccbs_tex);
		engine::CCBSplines *ccbs = new engine::CCBSplines(device);

		Effect *grow_fx = engine::loadEffect(device, "data/grow.fx");
		engine::Grow grow = engine::Grow(vertex_streamer, Vector3(-3.8f,-2.f,0.f));

		renderer::CubeTexture cubemap_tex = engine::loadCubeTexture(device, "data/stpeters_cross3.dds");
/*
		Effect test_fx = engine::loadEffect(device, "data/test.fx");
		test_fx->SetTexture("env", cube);
*/
		Effect *skybox_fx = engine::loadEffect(device, "data/skybox.fx");
		skybox_fx->setTexture("reflectionMap", cubemap_tex);
		Mesh *cube_x         = engine::loadMesh(device, "data/cube.X");

		Mesh *cornell_main_x         = engine::loadMesh(device, "data/cornell-main.x");
		Mesh *cornell_left_x         = engine::loadMesh(device, "data/cornell-left.x");
		Mesh *cornell_right_x         = engine::loadMesh(device, "data/cornell-right.x");
		Mesh *bart_x         = engine::loadMesh(device, "data/bart.x");
		Texture cornell_main_tex = engine::loadTexture(device, "data/cornell-main.jpg");
		Texture cornell_left_tex = engine::loadTexture(device, "data/cornell-left.jpg");
		Texture cornell_right_tex = engine::loadTexture(device, "data/cornell-right.jpg");

		Effect *tex_trans_fx       = engine::loadEffect(device, "data/tex_trans.fx");
		
		Surface logo_surf = loadSurface(device, "data/logo.png");
		Texture bar_tex = engine::loadTexture(device, "data/bar.png");
		
		Effect *tunelle_fx = engine::loadEffect(device, "data/tunelle.fx");
		tunelle_fx->setTexture("tex", engine::loadTexture(device, "data/tunelle.dds"));
		Mesh *tunelle_x = engine::loadMesh(device, "data/tunelle.x");
		
		Mesh *korridor_x = engine::loadMesh(device, "data/korridor.x");
		Effect *korridor_fx = engine::loadEffect(device, "data/korridor.fx");
		korridor_fx->setTexture("diffuse", engine::loadTexture(device, "data/korridor_diffuse.png"));
		korridor_fx->setTexture("lightmap", engine::loadTexture(device, "data/korridor_lightmap.png"));
		renderer::CubeTexture korridor_spherelight = engine::loadCubeTexture(device, "data/korridor_spherelight.dds");
		korridor_fx->setTexture("spherelight", korridor_spherelight);
		
		Mesh *korridor_sphere_x = engine::loadMesh(device, "data/korridor_sphere.x");
		Effect *korridor_sphere_fx = engine::loadEffect(device, "data/korridor_sphere.fx");
		korridor_sphere_fx->setTexture("lightmap", engine::loadTexture(device, "data/korridor_sphere_lightmap.png"));
		korridor_sphere_fx->setTexture("text", engine::loadTexture(device, "data/korridor_sphere_text.png"));
		
		Effect *korridor_particles_fx = engine::loadEffect(device, "data/korridor_particles.fx");
		korridor_particles_fx->setTexture("tex", lightparticle_tex);
		korridor_particles_fx->setTexture("spherelight", korridor_spherelight);
		
		renderer::CubeTexture greeble_envmap = engine::loadCubeTexture(device, "data/greeble_cube_env.dds");
		Mesh *smile_x = engine::loadMesh(device, "data/smile.x");
		Effect *smile_fx = engine::loadEffect(device, "data/smile.fx");
		smile_fx->setTexture("env", greeble_envmap);
		Effect *black_fx = engine::loadEffect(device, "data/black.fx");
		Effect *osc_fx = engine::loadEffect(device, "data/osc.fx");

		Anim greetingsAnim = engine::loadAnim(device, "data/greetings/");
		Image greetingsImage(greetingsAnim.getTexture(0), tex_fx);
		
		Anim beatAnim = engine::loadAnim(device, "data/beat/");
		Image beatImage(beatAnim.getTexture(0), tex_fx);

		Anim overlaysAnim = engine::loadAnim(device, "data/overlays/");
		Image overlaysImage(overlaysAnim.getTexture(0), tex_fx);

		Anim invmapAnim = engine::loadAnim(device, "data/invmaps/");

		Texture title_end_tex = engine::loadTexture(device, "data/title_end.png");
		Image titleEndSubtextImage(engine::loadTexture(device, "data/title_end_subtext.png"), tex_fx);
		
		BASS_Start();
		BASS_ChannelPlay(stream, false);
		BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, 0.0f));
		
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
			BASS_ChannelGetData(stream, osc, (OSC_SIZE * sizeof(float)) | BASS_DATA_FLOAT);
			device->BeginScene();
			
			/* setup multisampled stuffz */
			device.setRenderTarget(color_msaa.getRenderTarget());
			device.setDepthStencilSurface(depthstencil_msaa);
			
//			D3DXCOLOR clear_color(0.45f, 0.25f, 0.25f, 0.f);
			D3DXCOLOR clear_color(0.f, 0.f, 0.f, 0.f);
			
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
			
			at = Vector3(0, 0, 0);
			eye = Vector3(0, 0, -4);
			Vector3 orig_at = at;
			
			Matrix4x4 world = Matrix4x4::identity();
			Matrix4x4 view = Matrix4x4::lookAt(eye, at, roll);
			Matrix4x4 proj = Matrix4x4::projection(cameraFovTrack.getValue(beat), float(DEMO_ASPECT), 1.0f, 100000.f);
			
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

			bool skyboxEnabled = false;
			bool rayTraceEnabled = false;
			bool beatEnabled = false;
			bool smileEnabled = false;
			bool greetingsEnabled = false;
			bool cornellEnabled = false;
			
			int part = partTrack.getIntValue(beat);
			if (1 == part) {
				rayTraceEnabled = true;
			} else if (2 == part) {
				beatEnabled = true;
			} else if (4 == part) {
				greetingsEnabled = true;
			} else if (5 == part) {
				cornellEnabled = true;
			} else if (6 == part) {
				skyboxEnabled = true;
				smileEnabled = true;
			}

#if !SYNC
			if (-1 == part)
				done = true;
#endif

			float grid_size = 0.0f;
			int eye2_scroll2 = 0;
			float tunelle_scale = math::clamp((beat - (256 + 32)) * 0.5f, 0.0f, 1.0f);

			if (smileEnabled)
			{
				eye = Vector3(sin(cameraYRotTrack.getValue(beat) * (M_PI / 180)) * cameraDistanceTrack.getValue(beat),
					cameraUpTrack.getValue(beat),
					-cos(cameraYRotTrack.getValue(beat) * (M_PI / 180)) * cameraDistanceTrack.getValue(beat));

				at = Vector3(0, 0, 20);
				eye += at;
				
				view = Matrix4x4::lookAt(eye, at, roll);
			}

			if (skyboxEnabled)
			{
				if (smileEnabled)
				{
					skybox_fx->setTexture("reflectionMap", greeble_envmap);
					skybox_fx->setFloat("alpha", 0.25f);
				}
				else 
				{
					skybox_fx->setTexture("reflectionMap", cubemap_tex);
					skybox_fx->setFloat("alpha", 1.0f);
				}

				float scale = 100;
				Matrix4x4 world = Matrix4x4::scaling(Vector3(scale, scale, scale));
				skybox_fx->setMatrices(world, view, proj);
				skybox_fx->commitChanges();
			}

			// render
			for (int i = 0; i < 2; ++i)
			{
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
				
				device->Clear(0, 0, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, clear_color, 1.f, 0);
				if (skyboxEnabled) {
					device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
					skybox_fx->draw(cube_x);
					device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
				}


				//	test_fx.draw(cube_x);
				//	time /= 4;

/*				{
					testRenderer.view       = view;
					testRenderer.projection = proj; // math::Matrix4x4::projection(60.0f, 16.0f / 9, 1.0f, 1000.0f);
					testRenderer.draw();
				} */

				if (smileEnabled)
				{
					Matrix4x4 world = Matrix4x4::identity();
					at = Vector3(
						pow(sin(shake_time * 15 - cos(shake_time * 20)), 3),
						pow(cos(shake_time * 15 - sin(shake_time * 21)), 3),
						pow(cos(shake_time * 16 - sin(shake_time * 20)), 3)
						) * 0.025f * math::length(eye - at) * (cameraShakeAmtTrack.getValue(beat));
					view.makeLookAt(eye, at, roll);
					smile_fx->setMatrices(world, view, proj);
					smile_fx->commitChanges();
					
					black_fx->setMatrices(world, view, proj);
					black_fx->commitChanges();

					UINT passes = 0;
					smile_fx->p->Begin(&passes, 0);
					for (unsigned j = 0; j < passes; ++j)
					{
						smile_fx->p->BeginPass(j);
						smile_x->draw(j);
						smile_fx->p->EndPass();
					}
					smile_fx->p->End();
				}


				if (cornellEnabled)
				{
					Matrix4x4 world = Matrix4x4::identity();
					eye = Vector3(sin(cameraYRotTrack.getValue(beat) * (M_PI / 180)) * cameraDistanceTrack.getValue(beat),
							cameraUpTrack.getValue(beat),
							-cos(cameraYRotTrack.getValue(beat) * (M_PI / 180)) * cameraDistanceTrack.getValue(beat));
					at = Vector3(0, sphereOffsetTrack.getValue(beat), 0);
					view.makeLookAt(eye, at, roll);
					tex_trans_fx->setFloat("alpha", 1.0);

					world.makeIdentity();
					tex_trans_fx->setMatrices(world, view, proj);
					tex_trans_fx->setTexture("tex", cornell_main_tex);
					tex_trans_fx->commitChanges();
					tex_trans_fx->draw(cornell_main_x);

					float groove1 = 1.0f + sinf(beat * float(M_PI / 8)) * musGroove1Track.getValue(beat);
					float groove2 = 1.0f + sinf(beat * float(M_PI / 8)) * musGroove2Track.getValue(beat);

					world.makeScaling(Vector3(1, groove1, 1));
					tex_trans_fx->setMatrices(world, view, proj);
					tex_trans_fx->setTexture("tex", cornell_left_tex);
					tex_trans_fx->commitChanges();
					tex_trans_fx->draw(cornell_left_x);

					world.makeScaling(Vector3(1, groove2, 1));
					tex_trans_fx->setMatrices(world, view, proj);
					tex_trans_fx->setTexture("tex", cornell_right_tex);
					tex_trans_fx->commitChanges();
					tex_trans_fx->draw(cornell_right_x);


					float size1 = musSize1Track.getValue(beat) + (0.5f + cosf(beat * float(M_PI / 4)) * 0.5f) * musBoom1Track.getValue(beat);
					float size2 = musSize2Track.getValue(beat) + (0.5f + cosf(beat * float(M_PI / 4)) * 0.5f) * musBoom2Track.getValue(beat);
					world.makeScaling(Vector3(0.75, 0.75, 0.75) * size1);
					world *= Matrix4x4::rotation(Vector3(D3DXToRadian(168.765), 0, 0));
					world *= Matrix4x4::translation(Vector3(-15, 46 * groove1, -4.566));
					tex_trans_fx->setMatrices(world, view, proj);
					tex_trans_fx->draw(bart_x);

					world.makeScaling(Vector3(1.25, 1.25, 1.0) * size2);
					world *= Matrix4x4::rotation(Vector3(D3DXToRadian(-163.1), 0, 0));
					world *= Matrix4x4::translation(Vector3(13.264, 15 * groove2, -31));
					tex_trans_fx->setMatrices(world, view, proj);
					tex_trans_fx->draw(bart_x);
				}

				if (beatEnabled)
				{
					Texture &tex = beatAnim.getTexture(beatTrack.getIntValue(beat) % beatAnim.getTextureCount());
					beatImage.setTexture(tex);
					beatImage.setPosition(-1, -1);
					beatImage.setDimension(2, 2);
					beatImage.draw(device);
				}
				
				if (greetingsEnabled)
				{
					Texture &tex = greetingsAnim.getTexture(greetGroupTrack.getIntValue(beat) % greetingsAnim.getTextureCount());
/*					greetingsImage.setTexture(tex);
					greetingsImage.setPosition(-1, -1);
					greetingsImage.setDimension(2, 2);
					greetingsImage.draw(device); */

					tex_fx->setTexture("tex", tex);
					drawFuzz( tex_fx, vertex_streamer,  time, 1.0f,
						colorMapDistortXTrack.getValue(beat), colorMapDistortYTrack.getValue(beat),
						0.5f / letterbox_viewport.Width, 0.5f / letterbox_viewport.Height
					);
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
				/* explosion */
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
				device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
				device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
				explosion_fx->setMatrices(world, view, proj);
				explosion.draw(*explosion_fx, explosionTrack.getIntValue(beat));
				device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
#endif

				if (!i)
					drawOsc(osc_fx, vertex_streamer);
				
				device->SetRenderState(D3DRS_ZWRITEENABLE, true);
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
				
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
						 2.0f, 2.0f,
						0.5f / current_tex.getWidth(),
						0.5f / current_tex.getHeight()
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

			float flash = pow(colorMapFlashTrack.getValue(beat) == 1000.f ? (float)(rand()%2) : colorMapFlashTrack.getValue(beat), 2.0f);

			if (rayTraceEnabled) {
				eye = Vector3(sin(cameraYRotTrack.getValue(beat) * (M_PI / 180)) * cameraDistanceTrack.getValue(beat),
					cameraUpTrack.getValue(beat),
					-cos(cameraYRotTrack.getValue(beat) * (M_PI / 180)) * cameraDistanceTrack.getValue(beat));

				at = Vector3(0, 0, 0);
				eye += at;
				view = Matrix4x4::lookAt(eye, at, roll);
			}

			color_map_fx->setMatrices(world, view, proj);
			color_map_fx->setFloat("flash", flash);
			color_map_fx->setFloat("fade2", colorMapFadeTrack.getValue(beat));
			color_map_fx->setFloat("repeat", repeatTrack.getValue(beat));
			color_map_fx->setFloat("alpha", 0.0f);
			color_map_fx->p->SetBool("spheretracer", rayTraceEnabled);
			color_map_fx->setTexture("tex", color1_hdr);
			color_map_fx->setTexture("tex2", color_msaa);
			color_map_fx->setTexture("color_map", color_maps[colorMapPalTrack.getIntValue(beat) % 2]);
			color_map_fx->setTexture("invmap", invmapAnim.getTexture(invmapTrack.getIntValue(beat) % invmapAnim.getTextureCount()));
			color_map_fx->setTexture("overlay", overlaysAnim.getTexture(overlayTrack.getIntValue(beat) % overlaysAnim.getTextureCount()));
			
			drawFuzz(color_map_fx, vertex_streamer,  time, 1.0f, colorMapDistortXTrack.getValue(beat), colorMapDistortYTrack.getValue(beat),
				0.5f / letterbox_viewport.Width, 0.5f / letterbox_viewport.Height);
			
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
/*			scanlinesImage.setPosition(-1, -1);
			scanlinesImage.setDimension(2, 2);
			scanlinesImage.draw(device); */
			
/*			overlaysImage.setTexture(overlaysAnim.getTexture(overlayTrack.getIntValue(beat) % overlaysAnim.getTextureCount()));
			overlaysImage.setPosition(-1, -1);
			overlaysImage.setDimension(2, 2);
			overlaysImage.draw(device); */
			
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
				throw FatalException(std::string(DXGetErrorString(res)) + std::string(" : ") + std::string(DXGetErrorDescription(res)));
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
	
	return 0;
}
