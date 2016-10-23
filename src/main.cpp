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
#include "renderer/cubetexture.h"
#include "renderer/rendercubetexture.h"
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

#include "sync/sync.h"

#include "Box2D/Box2D.h"

using math::Vector2;
using math::Vector3;
using math::Matrix4x4;

using renderer::Device;
using renderer::Surface;
using renderer::Texture;
using renderer::CubeTexture;
using renderer::VolumeTexture;
using renderer::RenderTexture;
using renderer::RenderCubeTexture;

using engine::Mesh;
using engine::Effect;
using engine::Anim;

using namespace core;

#define D3DFMT_INTZ ((D3DFORMAT)MAKEFOURCC('I','N','T','Z'))

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

Matrix4x4 getCubemapViewMatrix(D3DCUBEMAP_FACES face)
{
	// Standard view that will be overridden below
	Vector3 vEnvEyePt = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 vLookatPt, vUpVec;

	switch(face) {
	case D3DCUBEMAP_FACE_POSITIVE_X:
		vLookatPt = Vector3(1.0f, 0.0f, 0.0f);
		vUpVec    = Vector3(0.0f, 1.0f, 0.0f);
		break;

	case D3DCUBEMAP_FACE_NEGATIVE_X:
		vLookatPt = Vector3(-1.0f, 0.0f, 0.0f);
		vUpVec    = Vector3( 0.0f, 1.0f, 0.0f);
		break;

	case D3DCUBEMAP_FACE_POSITIVE_Y:
		vLookatPt = Vector3(0.0f, 1.0f, 0.0f);
		vUpVec    = Vector3(0.0f, 0.0f,-1.0f);
		break;

	case D3DCUBEMAP_FACE_NEGATIVE_Y:
		vLookatPt = Vector3(0.0f,-1.0f, 0.0f);
		vUpVec    = Vector3(0.0f, 0.0f, 1.0f);
		break;

	case D3DCUBEMAP_FACE_POSITIVE_Z:
		vLookatPt = Vector3( 0.0f, 0.0f, 1.0f);
		vUpVec    = Vector3( 0.0f, 1.0f, 0.0f);
		break;

	case D3DCUBEMAP_FACE_NEGATIVE_Z:
		vLookatPt = Vector3(0.0f, 0.0f,-1.0f);
		vUpVec    = Vector3(0.0f, 1.0f, 0.0f);
		break;
	}

	Matrix4x4 view;
	D3DXMatrixLookAtLH(&view, &vEnvEyePt, &vLookatPt, &vUpVec);
	return view;
}

Matrix4x4 calcPlaneMatrix(Vector3 &v0, Vector3 &v1, Vector3 &v2)
{
	Vector3 a = v0 - v1;
	Vector3 b = v2 - v0;
	Vector3 n = cross(b, a);

	Matrix4x4 ret;
	ret._11 = a.x;
	ret._12 = a.y;
	ret._13 = a.z;
	ret._14 = 0;

	ret._21 = b.x;
	ret._22 = b.y;
	ret._23 = b.z;
	ret._24 = 0;

	ret._31 = n.x;
	ret._32 = n.y;
	ret._33 = n.z;
	ret._34 = 0;

	ret._41 = v0.x;
	ret._42 = v0.y;
	ret._43 = v0.z;
	ret._44 = 0;
	return ret;
}

void normalizePlane(float plane[4])
{
	float mag = sqrt(plane[0] * plane[0] +
	                 plane[1] * plane[1] +
	                 plane[2] * plane[2]);
	float scale = 1.0f / mag;
	for (int i = 0; i < 4; ++i)
		plane[i] = plane[i] * scale;
}

bool sphereInsideFrustum(const float frustum[6][4], const Vector3 &pos, float radius)
{
	for (int i = 0; i < 6; ++i) {
		float d = frustum[i][0] * pos.x +
		          frustum[i][1] * pos.y +
		          frustum[i][2] * pos.z +
		          frustum[i][3];

		if (d < -radius)
			return false;
	}

	return true;
}

std::vector<renderer::VolumeTexture> loadColorMaps(renderer::Device &device, std::string folder)
{
	const int MAP_SIZE = 32;

	renderer::Texture temp_tex = device.createTexture(MAP_SIZE * MAP_SIZE, MAP_SIZE, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED);
	std::vector<renderer::VolumeTexture> color_maps;
	for (int i = 0; true; ++i) {
		char temp[256];
		sprintf(temp, "%s/%04d.png", folder.c_str(), i);
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

	return color_maps;
}

std::vector<renderer::CubeTexture> loadSkyboxes(renderer::Device &device, std::string folder)
{
	std::vector<renderer::CubeTexture> ret;
	for (int i = 0; true; ++i) {
		char temp[256];
		sprintf(temp, "%s/%04d.dds", folder.c_str(), i);
		renderer::CubeTexture tex;
		if (FAILED(D3DXCreateCubeTextureFromFileEx(
			device,
			temp,
			D3DX_DEFAULT, // size
			D3DX_DEFAULT, // miplevels
			0, D3DFMT_UNKNOWN, // usage and format
			D3DPOOL_MANAGED, // pool
			D3DX_DEFAULT, D3DX_DEFAULT, // filtering
			0, NULL, NULL,
			&tex.tex)))
			break;

		ret.push_back(tex);
	}

	return ret;
}

std::vector<std::vector<Vector3> > loadSpheres(const char *path)
{
	FILE *fp = fopen(path, "rb");
	if (!fp)
		throw core::FatalException(std::string("failed to load ") + path);

	int spheres;
	if (fread(&spheres, sizeof(spheres), 1, fp) != 1)
		throw core::FatalException("read error");

	std::vector<std::vector<Vector3> > ret;
	for (int i = 0; i < spheres; ++i) {
		int keys;
		fread(&keys, sizeof(keys), 1, fp);
		std::vector<Vector3> curr;
		for (int j = 0; j < keys; ++j) {

			Vector3 tmp;
			if (fread(&tmp.x, sizeof(tmp.x), 1, fp) != 1 ||
			    fread(&tmp.y, sizeof(tmp.y), 1, fp) != 1 ||
			    fread(&tmp.z, sizeof(tmp.z), 1, fp) != 1)
				throw core::FatalException(std::string("read error: ") + strerror(ferror(fp)));

			curr.push_back(tmp);
		}
		ret.push_back(curr);
	}
	fclose(fp);

	return ret;
}

extern "C" _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

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
		stream = BASS_StreamCreateFile(false, "data/tune.mp3", 0, 0, BASS_MP3_SETPOS | BASS_STREAM_PRESCAN | ((0 == config::soundcard) ? BASS_STREAM_DECODE : 0));
		if (!stream)
			throw FatalException("failed to open tune");

		sync_device *rocket = sync_create_device("data/sync");
		if (!rocket)
			throw FatalException("something went wrong - failed to connect to host?");

#ifndef SYNC_PLAYER
		if (sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT))
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

		const sync_track *planeMoveTrack = sync_get_track(rocket, "plane.move");
		const sync_track *planeSizeTrack = sync_get_track(rocket, "plane.size");
		const sync_track *planeRotTrack = sync_get_track(rocket, "plane.rot");
		const sync_track *planeShapeTrack = sync_get_track(rocket, "plane.shape");
		const sync_track *planeStrokeTrack = sync_get_track(rocket, "plane.stroke");
		const sync_track *planeGreetingsTrack = sync_get_track(rocket, "plane.greet");
		const sync_track *planeTimeTrack = sync_get_track(rocket, "plane.time");
		const sync_track *planeFadeTrack = sync_get_track(rocket, "plane.fade");
		const sync_track *planeOverbrightTrack = sync_get_track(rocket, "plane.overbright");
		const sync_track *planeCountTrack = sync_get_track(rocket, "plane.count");
		const sync_track *planeOffsXTrack = sync_get_track(rocket, "plane.offs.x");
		const sync_track *planeOffsYTrack = sync_get_track(rocket, "plane.offs.y");
		const sync_track *planeOffsZTrack = sync_get_track(rocket, "plane.offs.z");


		const sync_track *colorMapFadeTrack    = sync_get_track(rocket, "cm.fade");
		const sync_track *colorMapFlashTrack   = sync_get_track(rocket, "cm.flash");
		const sync_track *colorMapOverlayTrack = sync_get_track(rocket, "cm.overlay");
		const sync_track *colorMapColormap1Track = sync_get_track(rocket, "cm.colormap1");
		const sync_track *colorMapColormap2Track = sync_get_track(rocket, "cm.colormap2");
		const sync_track *colorMapColorFadeTrack = sync_get_track(rocket, "cm.colorfade");
		const sync_track *colorMapOverlayAlphaTrack = sync_get_track(rocket, "cm.overlay_alpha");
		const sync_track *pulseAmt2Track       = sync_get_track(rocket, "cm.pulse.amt");
		const sync_track *pulseSpeed2Track     = sync_get_track(rocket, "cm.pulse.speed");
		const sync_track *lensDistTrack     = sync_get_track(rocket, "cm.lensdist");

		const sync_track *bloomCutoffTrack = sync_get_track(rocket, "bloom.cutoff");
		const sync_track *bloomShapeTrack  = sync_get_track(rocket, "bloom.shape");
		const sync_track *bloomAmtTrack    = sync_get_track(rocket, "bloom.amt");

		const sync_track *glitchBlockThreshTrack = sync_get_track(rocket, "glitch.blocks");
		const sync_track *glitchLineThreshTrack = sync_get_track(rocket, "glitch.lines");
		const sync_track *glitchOverlayTrack = sync_get_track(rocket, "glitch.overlay");

		const sync_track *flareAmountTrack = sync_get_track(rocket, "flare.amount");

		const sync_track *skyboxDesaturateTrack = sync_get_track(rocket, "skybox.desat");
		const sync_track *skyboxTextureTrack = sync_get_track(rocket, "skybox.tex");

		const sync_track *dofCocScaleTrack = sync_get_track(rocket, "dof.scale");
		const sync_track *dofFocalDistTrack = sync_get_track(rocket, "dof.fdist");

		const sync_track *groundYTrack = sync_get_track(rocket, "ground.y");

		const sync_track *spheresAnimTrack = sync_get_track(rocket, "spheres.anim");
		const sync_track *spheresDistTrack = sync_get_track(rocket, "spheres.dist");
		const sync_track *spheresPalTrack = sync_get_track(rocket, "spheres.pal");

		Surface backbuffer   = device.getRenderTarget(0);

		D3DCAPS9 caps;
		direct3d->GetDeviceCaps(config::adapter, D3DDEVTYPE_HAL, &caps);

		// 0: XYZ = normal, W = unused
		// 1: XYZ = albedo, W = unused
		RenderTexture gbuffer_target0(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);
		RenderTexture gbuffer_target1(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A8R8G8B8);

		RenderTexture color_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);
		RenderTexture depth_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_INTZ, D3DMULTISAMPLE_NONE, D3DUSAGE_DEPTHSTENCIL);

		RenderTexture dof_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);
		RenderTexture dof_temp1_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);
		RenderTexture dof_temp2_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);

		RenderTexture fxaa_target(device, letterbox_viewport.Width, letterbox_viewport.Height, 1, D3DFMT_A16B16G16R16F);

		RenderTexture color1_hdr(device, letterbox_viewport.Width, letterbox_viewport.Height, 0, D3DFMT_A16B16G16R16F);
		RenderTexture color2_hdr(device, letterbox_viewport.Width, letterbox_viewport.Height, 0, D3DFMT_A16B16G16R16F);
		RenderTexture flare_tex(device, letterbox_viewport.Width / 4, letterbox_viewport.Height / 4, 1, D3DFMT_A16B16G16R16F);

		RenderTexture logo_anim_target(device, 512, 512, 0, D3DFMT_A16B16G16R16F, D3DMULTISAMPLE_NONE, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP);

		RenderCubeTexture reflection_target(device, 512, 1, D3DFMT_A16B16G16R16F);
		Surface reflection_depthstencil = device.createDepthStencilSurface(512, 512, D3DFMT_D24S8);

		std::vector<renderer::VolumeTexture> color_maps = loadColorMaps(device, "data/color_maps");
		std::vector<renderer::CubeTexture> skyboxes = loadSkyboxes(device, "data/skyboxes");

		Effect *dof_fx = engine::loadEffect(device, "data/dof.fx");
		dof_fx->setVector3("viewport", Vector3(letterbox_viewport.Width, letterbox_viewport.Height, 0.0f));

		Effect *blur_fx      = engine::loadEffect(device, "data/blur.fx");

		Effect *fxaa_fx = engine::loadEffect(device, "data/fxaa.fx");
		fxaa_fx->setVector3("viewportInv", Vector3(1.0f / letterbox_viewport.Width, 1.0f / letterbox_viewport.Height, 0.0f));

		Effect *flare_fx = engine::loadEffect(device, "data/flare.fx");
		flare_fx->setVector3("viewport", Vector3(letterbox_viewport.Width, letterbox_viewport.Height, 0.0f));

		Effect *postprocess_fx = engine::loadEffect(device, "data/postprocess.fx");
		postprocess_fx->setVector3("viewport", Vector3(letterbox_viewport.Width, letterbox_viewport.Height, 0.0f));
		Texture lensdirt_tex = engine::loadTexture(device, "data/lensdirt.png");
		Texture vignette_tex = engine::loadTexture(device, "data/vignette.png");
		postprocess_fx->setTexture("lensdirt_tex", lensdirt_tex);
		postprocess_fx->setTexture("vignette_tex", vignette_tex);


		Texture noise_tex = engine::loadTexture(device, "data/noise.png");
		postprocess_fx->setTexture("noise_tex", noise_tex);
		postprocess_fx->setVector3("nscale", Vector3(letterbox_viewport.Width / 256.0f, letterbox_viewport.Height / 256.0f, 0.0f));

		Texture spectrum_tex = engine::loadTexture(device, "data/spectrum.png");
		flare_fx->setTexture("spectrum_tex", spectrum_tex);
		postprocess_fx->setTexture("spectrum_tex", spectrum_tex);

		engine::ParticleStreamer particleStreamer(device);
		Effect *particle_fx = engine::loadEffect(device, "data/particle.fx");

		Mesh *skybox_x = engine::loadMesh(device, "data/skybox.x");
		Effect *skybox_fx = engine::loadEffect(device, "data/skybox.fx");

		Anim overlays = engine::loadAnim(device, "data/overlays");

		Effect *mesh_fx = engine::loadEffect(device, "data/mesh.fx");

		Texture con_diffuse_tex = engine::loadTexture(device, "data/Con_Diffuse_1.jpg");
		Texture con_normal_tex = engine::loadTexture(device, "data/Con_Normal_1.jpg");
		Texture con_specular_tex = engine::loadTexture(device, "data/Con_Specular_1.png");

		Effect *corridor_fx = engine::loadEffect(device, "data/corridor.fx");
		corridor_fx->setTexture("albedo_tex", con_diffuse_tex);
		corridor_fx->setTexture("normal_tex", con_normal_tex);
		corridor_fx->setTexture("specular_tex", con_specular_tex);

		Effect *corridor_dark_fx = engine::loadEffect(device, "data/corridor-dark.fx");
		corridor_dark_fx->setTexture("albedo_tex", con_diffuse_tex);
		corridor_dark_fx->setTexture("normal_tex", con_normal_tex);
		corridor_dark_fx->setTexture("specular_tex", con_specular_tex);

		Effect *corridor_floor_fx = engine::loadEffect(device, "data/corridor-floor.fx");
		corridor_floor_fx->setTexture("albedo_tex", con_diffuse_tex);
		corridor_floor_fx->setTexture("normal_tex", con_normal_tex);
		corridor_floor_fx->setTexture("specular_tex", con_specular_tex);

		Mesh *corridor1_x = engine::loadMesh(device, "data/corridor1.x");
		Mesh *corridor1_dark_x = engine::loadMesh(device, "data/corridor1-dark.x");
		Mesh *corridor1_floor_x = engine::loadMesh(device, "data/corridor1-floor.x");
		Texture corridor1_ao_tex = engine::loadTexture(device, "data/corridor1_ao.png");

		Effect *lighting_fx = engine::loadEffect(device, "data/lighting.fx");
		lighting_fx->setTexture("logo_tex", logo_anim_target);

		Effect *sphere_fx = engine::loadEffect(device, "data/sphere.fx");
		Texture kulefarger_tex = engine::loadTexture(device, "data/kulefarger.png");
		sphere_fx->setTexture("colors_tex", kulefarger_tex);

		Effect *plane_fx = engine::loadEffect(device, "data/plane.fx");

		Mesh *groundplane_x = engine::loadMesh(device, "data/groundplane.x");
		Effect *groundplane_fx = engine::loadEffect(device, "data/groundplane.fx");

		Texture ground_albedo_tex = engine::loadTexture(device, "data/concrete_01_dif.png");
		Texture ground_normal_tex = engine::loadTexture(device, "data/concrete_01_nm.png");
		Texture ground_specular_tex = engine::loadTexture(device, "data/concrete_01_spec.png");

		groundplane_fx->setTexture("albedo_tex", ground_albedo_tex);
		groundplane_fx->setTexture("normal_tex", ground_normal_tex);
		groundplane_fx->setTexture("specular_tex", ground_specular_tex);

		Effect *logo_anim_fx = engine::loadEffect(device, "data/logo-anim.fx");
		Anim shapes = engine::loadAnim(device, "data/shapes");
		Anim strokes = engine::loadAnim(device, "data/strokes");
		Anim greetings = engine::loadAnim(device, "data/greetings");

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
		DWORD startTick = GetTickCount();
		double prevTime = startTick / 1000.0;
		while (!done) {
			if (dump_video) {
				QWORD pos = BASS_ChannelSeconds2Bytes(stream, float(frame) / config::mode.RefreshRate);
				BASS_ChannelSetPosition(stream, pos, BASS_POS_BYTE);
			}

			double row = bass_get_row(stream);
			double time = (GetTickCount() - startTick) / 1000.0;
			double deltaTime = time - prevTime;
			prevTime = time;

#ifndef SYNC_PLAYER
			sync_update(rocket, int(row), &bass_cb, (void *)stream);
#endif
			double beat = row / 4;

			double camTime = sync_get_val(cameraTimeTrack, row);
			double camOffset = sync_get_val(cameraOffsetTrack, row);
			Vector3 camPos, camTarget, camUp = Vector3(0, 1, 0);
			switch ((int)sync_get_val(cameraIndexTrack, row)) {
			case 0:
				camTarget = Vector3(sync_get_val(cameraAtXTrack, row), sync_get_val(cameraAtYTrack, row), sync_get_val(cameraAtZTrack, row));
				camPos = Vector3(sin(camTime / 2) * sync_get_val(cameraDistanceTrack, row),
					sync_get_val(cameraYTrack, row),
					cos(camTime / 2) * sync_get_val(cameraDistanceTrack, row));
				break;

			case 1:
				camPos = Vector3(sin(camTime * float(M_PI / 180)), cos(camTime * float(M_PI / 180)), 0) * float(sync_get_val(cameraDistanceTrack, row));
				camTarget = Vector3(sin((camTime + camOffset) * float(M_PI / 180)), cos((camTime + camOffset) * float(M_PI / 180)), 0) * float(sync_get_val(cameraDistanceTrack, row));
				camUp = camPos - camTarget;
				camUp = Vector3(camUp.y, camUp.z, camUp.x);
				break;

			case 2: {
				double angle = sync_get_val(cameraTimeTrack, row) * float(M_PI / 180);
				double angle2 = angle + sync_get_val(cameraOffsetTrack, row) * float(M_PI / 180);
				camPos = Vector3(sin(angle) * 30, 0, cos(angle) * 30);
				camPos += normalize(camPos) * float(sync_get_val(cameraYTrack, row));
				camTarget = Vector3(sin(angle2) * 30, 0, cos(angle2) * 30);
				camTarget += normalize(camTarget) * float(sync_get_val(cameraYTrack, row));
				} break;

			case 3:
				camPos = Vector3(sync_get_val(cameraXTrack, row), sync_get_val(cameraYTrack, row), sync_get_val(cameraZTrack, row));
				camTarget = Vector3(sync_get_val(cameraAtXTrack, row), sync_get_val(cameraAtYTrack, row), sync_get_val(cameraAtZTrack, row));
				break;

			default:
				camPos = Vector3(0, 1, 0) * float(sync_get_val(cameraDistanceTrack, row));
				camTarget = Vector3(0, 0, 0);
			}

			float zNear = 0.1f;
			float zFar = 1000.0f;
			Vector2 nearFar((zFar - zNear) / -(zFar * zNear),
			                 1.0f / zNear);

			float focal_distance = float(sync_get_val(dofFocalDistTrack, row));
			float coc_scale = float(sync_get_val(dofCocScaleTrack, row) / letterbox_viewport.Height);
			dof_fx->setFloat("focal_distance", focal_distance);
			dof_fx->setFloat("coc_scale", coc_scale);
			dof_fx->setVector2("nearFar", nearFar);
			particle_fx->setFloat("focal_distance", focal_distance);
			particle_fx->setFloat("coc_scale", coc_scale);

			bool dof = true;
			bool reflectionPlane = false;
			bool groundplane = false;
			bool corridor = false;
			bool sphereSphere = false;
			bool sphereColumn = false;
			int dustParticleCount = 0;
			Vector3 fogColor(0, 0, 0);

			int part = int(sync_get_val(partTrack, row));
			switch (part) {
			case 0:
				groundplane = true;
				reflectionPlane = true;
				break;

			case 1:
				corridor = true;
				fogColor = Vector3(0.01, 0.01, 0.01);
				reflectionPlane = true;
				break;

			case 2:
				sphereSphere = true;
				reflectionPlane = true;
				break;

			case 3:
				corridor = true;
				fogColor = Vector3(0.01, 0.01, 0.01);
				sphereColumn = true;
				reflectionPlane = true;
				break;
			}

#ifdef SYNC_PLAYER
			if (part < 0)
				done = true;
#endif

			double shake_phase = beat * 32 * sync_get_val(cameraShakeSpeedTrack, row);
			Vector3 camOffs(sin(shake_phase), cos(shake_phase * 0.9), sin(shake_phase - 0.5));
			camPos += camOffs * float(sync_get_val(cameraShakeAmtTrack, row));
			camTarget += camOffs * float(sync_get_val(cameraShakeAmtTrack, row));

			double camRoll = sync_get_val(cameraRollTrack, row) * (M_PI / 180);
			Matrix4x4 view;
			D3DXMatrixLookAtLH(&view, &camPos, &camTarget, &camUp);
			view *= Matrix4x4::rotation(Vector3(0, 0, camRoll));

			Matrix4x4 world = Matrix4x4::identity();
			Matrix4x4 proj  = Matrix4x4::projection(80.0f, float(DEMO_ASPECT), zNear, zFar);

			// render
			device->BeginScene();
			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

			device.setRenderTarget(logo_anim_target.getRenderTarget(), 0);
			device.setRenderTarget(NULL, 1);
			device->Clear(0, 0, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0xFF000000, 1.f, 0);

			logo_anim_fx->setTexture("shape_tex", shapes.getTexture(int(sync_get_val(planeShapeTrack, row)) % shapes.getTextureCount()));
			logo_anim_fx->setTexture("stroke_tex", strokes.getTexture(int(sync_get_val(planeStrokeTrack, row)) % strokes.getTextureCount()));
			logo_anim_fx->setTexture("greetings_tex", greetings.getTexture(int(sync_get_val(planeGreetingsTrack, row)) % greetings.getTextureCount()));
			logo_anim_fx->setFloat("time", float(fmod((sync_get_val(planeTimeTrack, row) + 0.5) / 512, 1)));
			logo_anim_fx->setFloat("fade", float(sync_get_val(planeFadeTrack, row)));
			drawQuad(device, logo_anim_fx, -1, -1, 2, 2);

			device.setRenderTarget(color_target.getRenderTarget(), 0);
			device.setDepthStencilSurface(depth_target.getRenderTarget());
			device->SetRenderState(D3DRS_ZENABLE, true);

			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			device->SetRenderState(D3DRS_ZWRITEENABLE, true);

			device->Clear(0, 0, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0xFF000000, 1.f, 0);

			int skybox = (int)sync_get_val(skyboxTextureTrack, row);
			if (skybox >= 0 && skybox < (int)skyboxes.size()) {
				skybox_fx->setMatrices(world, view, proj);
				skybox_fx->setFloat("desaturate", float(sync_get_val(skyboxDesaturateTrack, row)));
				skybox_fx->setTexture("env_tex", skyboxes[skybox]);
				skybox_fx->commitChanges();
				skybox_fx->draw(skybox_x);
			}

			device.setRenderTarget(gbuffer_target0.getRenderTarget(), 0);
			device.setRenderTarget(gbuffer_target1.getRenderTarget(), 1);
			// clear GBuffer
			device->Clear(0, 0, D3DCLEAR_TARGET, 0xFF000000, 1.f, 0);

			if (groundplane) {
				Matrix4x4 world = Matrix4x4::translation(Vector3(0, float(sync_get_val(groundYTrack, row)), 0));

				groundplane_fx->setMatrices(world, view, proj);
				groundplane_fx->draw(groundplane_x);
			}

			if (corridor) {
				for (int i = 0; i < 10; ++i) {
					Matrix4x4 world = Matrix4x4::translation(Vector3(0, 0, i * 315));
					corridor_fx->setMatrices(world, view, proj);
					corridor_fx->setTexture("ao_tex", corridor1_ao_tex);
					corridor_fx->draw(corridor1_x);

					corridor_dark_fx->setMatrices(world, view, proj);
					corridor_dark_fx->setTexture("ao_tex", corridor1_ao_tex);
					corridor_dark_fx->draw(corridor1_dark_x);

					corridor_floor_fx->setMatrices(world, view, proj);
					corridor_floor_fx->setTexture("ao_tex", corridor1_ao_tex);
					corridor_floor_fx->draw(corridor1_floor_x);
				}
			}

			if (sphereSphere || sphereColumn) {
				sphere_fx->setMatrices(world, view, proj);
				sphere_fx->setVector2("nearFar", nearFar);
				sphere_fx->setVector2("viewport", Vector2(letterbox_viewport.Width, letterbox_viewport.Height));

				math::Matrix4x4 worldViewProj = world * view * proj;

				float frustum[6][4];
				worldViewProj.extractFrustumPlanes(frustum);
				for (int i = 0; i < 6; ++i)
					normalizePlane(frustum[i]);

				float anim = float(sync_get_val(spheresAnimTrack, row));
				float dist = float(sync_get_val(spheresDistTrack, row));
				float pal = float(0.5f + sync_get_val(spheresPalTrack, row)) / kulefarger_tex.getHeight();

				struct Sphere {
					Vector3 pos;
					float size;
					Vector3 color;
				};

				std::vector<Sphere> spheres;
				if (sphereSphere) {
					spheres.resize(3600);
					for (size_t i = 0; i < spheres.size(); ++i) {
						float s = math::notRandf(i * 3 + 0) * float(M_PI * 2);
						float t = math::notRandf(i * 3 + 1) * 2 - 1;
						float l = sqrt(1 - t * t);
						Vector3 pos = Vector3(sin(s) * l, cos(s) * l, t) * 70;
						Vector3 offset = normalize(Vector3(
								sin((i % 1337) * 12.0 + anim * 0.0332),
								cos((i % 1338) * 15.0 + anim * 0.041),
								cos((i % 1339) * 13.0 - anim * 0.0323)
								));
						pos += offset * dist;
						float size = 0.2f + pow(math::notRandf(i), 5.0f) * 0.75f;
						spheres[i].pos = pos;
						spheres[i].size = size * 10;

						int color_idx = int(math::notRandf(i * 3 + 2) * kulefarger_tex.getWidth());
						float color = (0.5f + color_idx) / kulefarger_tex.getWidth();
						spheres[i].color = Vector3(color,
						                           pal,
						                           0.0f);
					}
				} else {
					spheres.resize(3600);
					for (size_t i = 0; i < spheres.size(); ++i) {
						float s = math::notRandf(i * 3 + 0) * float(M_PI * 2);
						float t = math::notRandf(i * 3 + 1) * 2 - 1;
						Vector3 pos = Vector3(sin(s), cos(s), i);
						Vector3 offset = normalize(Vector3(
								sin((i % 1337) * 12.0 + anim * 0.0332),
								cos((i % 1338) * 15.0 + anim * 0.041),
								cos((i % 1339) * 13.0 - anim * 0.0323)
								));
						pos += offset * dist;
						float size = 0.2f + pow(math::notRandf(i), 5.0f) * 0.75f;
						spheres[i].pos = pos;
						spheres[i].size = size * 10;

						int color_idx = int(math::notRandf(i * 3 + 2) * kulefarger_tex.getWidth());
						float color = (0.5f + color_idx) / kulefarger_tex.getWidth();
						spheres[i].color = Vector3(color,
												   pal,
												   0.0f);
					}
				}

				particleStreamer.begin();
				for (size_t i = 0; i < spheres.size(); ++i) {
					Vector3 pos = spheres[i].pos;
					float size = spheres[i].size;

					if (!sphereInsideFrustum(frustum, pos, size))
						continue;

					particleStreamer.add(pos, size, spheres[i].color);
					if (!particleStreamer.getRoom()) {
						particleStreamer.end();
						sphere_fx->drawPass(&particleStreamer, 0);
						particleStreamer.begin();
					}
				}
				particleStreamer.end();
				sphere_fx->drawPass(&particleStreamer, 0);

				device.setRenderTarget(NULL, 1);
				device.setRenderTarget(gbuffer_target1.getRenderTarget(), 0);

				sphere_fx->setTexture("depth_tex", depth_target);
				sphere_fx->setTexture("gbuffer_tex0", gbuffer_target0);
				sphere_fx->setTexture("gbuffer_tex1", gbuffer_target1);

				particleStreamer.begin();
				for (size_t i = 0; i < spheres.size(); ++i) {
					Vector3 pos = spheres[i].pos;
					float size = spheres[i].size * 3;

					if (!sphereInsideFrustum(frustum, pos, size))
						continue;

					particleStreamer.add(pos, spheres[i].size, spheres[i].color);
					if (!particleStreamer.getRoom()) {
						particleStreamer.end();
						sphere_fx->drawPass(&particleStreamer, 1);
						particleStreamer.begin();
					}
				}
				particleStreamer.end();
				sphere_fx->drawPass(&particleStreamer, 1);
			}

			float size = float(sync_get_val(planeSizeTrack, row));
			float plane_distance = float(sync_get_val(planeMoveTrack, row));
			float plane_rot = float(sync_get_val(planeRotTrack, row));
			int planeCount = int(sync_get_val(planeCountTrack, row));
			const int MAX_PLANES = 4;
			planeCount = math::clamp(planeCount, int(0), MAX_PLANES);
			Vector3 planeOffset(float(sync_get_val(planeOffsXTrack, row)),
			                    float(sync_get_val(planeOffsYTrack, row)),
			                    float(sync_get_val(planeOffsZTrack, row)));
			Vector3 planeOrigin = -(planeOffset * float(planeCount - 1)) / 2;

			Matrix4x4 planeRotation = Matrix4x4::rotation(Vector3(0, 0, plane_rot * float(M_PI / 180)));

			Matrix4x4 planeMatrices[MAX_PLANES];
			D3DXVECTOR4 planeVertices[MAX_PLANES * 4];
			for (int i = 0; i < planeCount; ++i) {
				Matrix4x4 planeTranslation = Matrix4x4::translation(planeOrigin + planeOffset * float(i));
				Matrix4x4 planeTransform = (planeRotation * planeTranslation) * view;
				Vector3 v0 = mul(planeTransform, Vector3(-size, -size, plane_distance));
				Vector3 v1 = mul(planeTransform, Vector3( size, -size, plane_distance));
				Vector3 v2 = mul(planeTransform, Vector3(-size,  size, plane_distance));
				Vector3 v3 = mul(planeTransform, Vector3( size,  size, plane_distance));

				planeMatrices[i] = calcPlaneMatrix(v0, v1, v2);
				planeVertices[i * 4 + 0] = D3DXVECTOR4(v1.x, v1.y, v1.z, 1);
				planeVertices[i * 4 + 1] = D3DXVECTOR4(v3.x, v3.y, v3.z, 1);
				planeVertices[i * 4 + 2] = D3DXVECTOR4(v2.x, v2.y, v2.z, 1);
				planeVertices[i * 4 + 3] = D3DXVECTOR4(v0.x, v0.y, v0.z, 1);
			}
			lighting_fx->p->SetInt("planeCount", planeCount);
			lighting_fx->p->SetVectorArray("planeVertices", planeVertices, 4 * planeCount);
			lighting_fx->setFloat("planeOverbright", float(sync_get_val(planeOverbrightTrack, row)));
			lighting_fx->setVector3("fogColor", fogColor);
			lighting_fx->setFloat("fogDensity", 0.01f);

			device.setRenderTarget(color_target.getRenderTarget(), 0);
			device.setRenderTarget(NULL, 1);
			lighting_fx->setMatrices(world, view, proj);
			lighting_fx->setTexture("depth_tex", depth_target);
			lighting_fx->setTexture("gbuffer_tex0", gbuffer_target0);
			lighting_fx->setTexture("gbuffer_tex1", gbuffer_target1);
			lighting_fx->setVector2("nearFar", nearFar);
			lighting_fx->p->SetMatrixArray("planeMatrices", planeMatrices, planeCount);
			drawQuad(device, lighting_fx, -1, -1, 2, 2);

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

			device.setDepthStencilSurface(depth_target.getRenderTarget());

			if (reflectionPlane) {
				for (int i = 0; i < planeCount; ++i) {
					Matrix4x4 world = planeRotation * Matrix4x4::translation(planeOrigin + planeOffset * float(i) + Vector3(0, 0, plane_distance));
					plane_fx->setMatrices(world, view, proj);
					plane_fx->setTexture("albedo_tex", logo_anim_target);
					drawQuad(device, plane_fx, -size, -size, size * 2, size * 2);
				}
			}

			if (dustParticleCount > 0) {
				Matrix4x4 modelview = world * view;
				Vector3 up(modelview._12, modelview._22, modelview._32);
				Vector3 left(modelview._11, modelview._21, modelview._31);
				math::normalize(up);
				math::normalize(left);
				particle_fx->setVector3("up", up);
				particle_fx->setVector3("left", left);
				particle_fx->setMatrices(world, view, proj);
				particle_fx->setVector2("viewport", Vector2(letterbox_viewport.Width, letterbox_viewport.Height));

				device.setRenderTarget(dof_target.getSurface(0), 0);

				particleStreamer.begin();
				for (int i = 0; i < dustParticleCount; ++i) {
					Vector3 pos = Vector3(math::notRandf(i) * 2 - 1, math::notRandf(i + 1) * 2 - 1, math::notRandf(i + 2) * 2 - 1) * 30;
					Vector3 offset = normalize(Vector3(
							sin(i * 0.23 + beat * 0.0532),
							cos(i * 0.27 + beat * 0.0521),
							cos(i * 0.31 - beat * 0.0512)
							));
					pos += offset * 3;
					float size = math::notRandf(i * 3 + 1) * 2.5f;
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

			device.setRenderTarget(fxaa_target.getSurface(0), 0);
			device.setRenderTarget(color1_hdr.getSurface(), 1);
			fxaa_fx->setTexture("color_tex", dof ? dof_target : color_target);
			fxaa_fx->setFloat("bloom_cutoff", float(sync_get_val(bloomCutoffTrack, row)));
			drawQuad(device, fxaa_fx, -1, -1, 2, 2);
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
					drawQuad(device, blur_fx, -1, -1, 2, 2);
				}
			}

			device.setRenderTarget(flare_tex.getSurface(0));
			flare_fx->setTexture("bloom_tex", color1_hdr);
			drawQuad(device, flare_fx, -1, -1, 2, 2);

			/* letterbox */
			device.setRenderTarget(backbuffer);
			device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.f, 0);
			device.setViewport(&letterbox_viewport);

			float flash = float(sync_get_val(colorMapFlashTrack, row));
			float fade = float(sync_get_val(colorMapFadeTrack, row));
			float pulse = float(sync_get_val(pulseAmt2Track, row));
			fade = std::max(0.0f, fade - pulse + float(cos(beat * sync_get_val(pulseSpeed2Track, row) * M_PI)) * pulse);
			postprocess_fx->setVector3("noffs", Vector3(math::notRandf(int(beat * 100)), math::notRandf(int(beat * 100) + 1), 0));
			postprocess_fx->setFloat("flash", flash < 0 ? math::randf() : pow(flash, 2.0f));
			postprocess_fx->setFloat("fade", pow(fade, 2.2f));
			postprocess_fx->setVector2("dist_offset", Vector2(1234.0, 3543.0) * float(beat * 4));
			postprocess_fx->setTexture("color_tex", fxaa_target);
			postprocess_fx->setFloat("overlay_alpha", float(sync_get_val(colorMapOverlayAlphaTrack, row)));
			postprocess_fx->setTexture("overlay_tex", overlays.getTexture(int(sync_get_val(colorMapOverlayTrack, row)) % overlays.getTextureCount()));
			postprocess_fx->setTexture("bloom_tex", color1_hdr);
			postprocess_fx->setTexture("flare_tex", flare_tex);
			postprocess_fx->setFloat("block_thresh", float(sync_get_val(glitchBlockThreshTrack, row)));
			postprocess_fx->setFloat("line_thresh", float(sync_get_val(glitchLineThreshTrack, row)));
			postprocess_fx->setFloat("overlayGlitch", float(sync_get_val(glitchOverlayTrack, row)));

			postprocess_fx->setFloat("flare_amount", float(sync_get_val(flareAmountTrack, row)));
			postprocess_fx->setFloat("distCoeff", float(sync_get_val(lensDistTrack, row)));

			float bloom_shape = float(sync_get_val(bloomShapeTrack, row));
			float bloom_weight[7];
			float bloom_total = 0;
			for (int i = 0; i < 7; ++i) {
				bloom_weight[i] = powf(float(i), bloom_shape);
				bloom_total += bloom_weight[i];
			}
			float bloom_scale = float(sync_get_val(bloomAmtTrack, row) / bloom_total);
			for (int i = 0; i < 7; ++i)
				bloom_weight[i] *= bloom_scale;
			postprocess_fx->setFloatArray("bloom_weight", bloom_weight, ARRAY_SIZE(bloom_weight));

			postprocess_fx->setTexture("color_map1_tex", color_maps[(int)sync_get_val(colorMapColormap1Track, row) % color_maps.size()]);
			postprocess_fx->setTexture("color_map2_tex", color_maps[(int)sync_get_val(colorMapColormap2Track, row) % color_maps.size()]);
			postprocess_fx->setFloat("color_map_lerp", float(sync_get_val(colorMapColorFadeTrack, row)));

			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			drawQuad(device, postprocess_fx, -1, -1, 2, 2);

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

#ifndef SYNC_PLAYER
				if (WM_KEYDOWN == msg.message) {
					switch (LOWORD(msg.wParam)) {
					case 'R':
						log::printf("reloading color maps");
						color_maps = loadColorMaps(device, "data/color_maps");
						break;

					case 'O':
						log::printf("reloading overlays");
						overlays = engine::loadAnim(device, "data/overlays");
						break;

					case 'P':
						shapes = engine::loadAnim(device, "data/shapes");
						strokes = engine::loadAnim(device, "data/strokes");
						break;
					}
				}
#endif

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
#ifndef SYNC_PLAYER
		sync_save_tracks(rocket);
#endif
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
