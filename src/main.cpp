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

#include "sync/sync.h"

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

using namespace core;

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

const int rpb = 8; /* rows per beat */
const double row_rate = (double(BPM) / 60) * rpb;

double bass_get_row(HSTREAM h)
{
	QWORD pos = BASS_ChannelGetPosition(h, BASS_POS_BYTE);
	double time = BASS_ChannelBytes2Seconds(h, pos);
	return time * row_rate + 0.005;
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
		stream = BASS_StreamCreateFile(false, "data/tune.mp3", 0, 0, BASS_MP3_SETPOS | BASS_STREAM_PRESCAN | ((0 == config::soundcard) ? BASS_STREAM_DECODE : 0));
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
		const sync_track *cameraRollTrack     = sync_get_track(rocket, "cam.roll");
		const sync_track *cameraOffsetTrack   = sync_get_track(rocket, "cam.offset");
		const sync_track *cameraIndexTrack    = sync_get_track(rocket, "cam.index");

		const sync_track *colorMapBlendTrack  = sync_get_track(rocket, "cm.blend");
		const sync_track *colorMapFadeTrack   = sync_get_track(rocket, "cm.fade");
		const sync_track *colorMapFlashTrack  = sync_get_track(rocket, "cm.flash");
		const sync_track *bloomSizeTrack      = sync_get_track(rocket, "bloom.size");
		const sync_track *bloomPassesTrack    = sync_get_track(rocket, "bloom.passes");
		const sync_track *bloomAmtTrack       = sync_get_track(rocket, "bloom.amt");

		const sync_track *fogDensityTrack     = sync_get_track(rocket, "fog.density");

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
		RenderTexture cube_light_tex(device, 32, 32, 1, use_sm20_codepath ? D3DFMT_A8R8G8B8 : D3DFMT_A16B16G16R16F);
		RenderTexture color1_hdr(device, 1280 / 2, int((1280 / DEMO_ASPECT) / 2), 1,
		    use_sm20_codepath ? D3DFMT_A8R8G8B8 : D3DFMT_A16B16G16R16F);
		RenderTexture color2_hdr(device, 1280 / 2, int((1280 / DEMO_ASPECT) / 2), 1,
		    use_sm20_codepath ? D3DFMT_A8R8G8B8 : D3DFMT_A16B16G16R16F);

		Effect *blur_fx     = engine::loadEffect(device, "data/blur.fx");
		Effect *color_map_fx = engine::loadEffect(device, "data/color_map.fx");

		Texture noise_tex = engine::loadTexture(device, "data/noise.png");
		color_map_fx->setTexture("noise_tex", noise_tex);
		if (use_sm20_codepath)
			color_map_fx->p->SetTechnique("rgbe");

		Mesh *cube_tops_x  = engine::loadMesh(device, "data/cube-grid-tops-32.x");
		Mesh *cube_sides_x = engine::loadMesh(device, "data/cube-grid-sides-32.x");
		Mesh *cube_floor_x = engine::loadMesh(device, "data/cube-grid-floor.x");
		Effect *cube_light_fx = engine::loadEffect(device, "data/cube-light.fx");
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

		BASS_Start();
		BASS_ChannelPlay(stream, false);

		// todo: config this
		bool dump_video = false;
		float video_framerate = 60.0f;

		bool done = false;
		int frame = 0;
		while (!done) {
			if (dump_video) {
				QWORD pos = BASS_ChannelSeconds2Bytes(stream, frame / video_framerate);
				BASS_ChannelSetPosition(stream, pos, BASS_POS_BYTE);
			}

			double row = bass_get_row(stream);

#ifndef SYNC_PLAYER
			sync_update(rocket, int(row)); //gets current timing info from the SyncTimer.
#endif
			double beat = row / 4;

			float camTime = float(beat / 4) + sync_get_val(cameraOffsetTrack, row);
			Vector3 camPos(
				sin(camTime * 0.25f) * 1.5,
				1.0,
				cos(camTime * 0.33f) * 1.5);
			camPos *= sync_get_val(cameraDistanceTrack, row);
			Vector3 camTarget(0, 0, 0);

			float camRoll = sync_get_val(cameraRollTrack, row) * float(2 * M_PI);
			Matrix4x4 view  = Matrix4x4::lookAt(camPos, camTarget, camRoll);
			Matrix4x4 world = Matrix4x4::identity();
			Matrix4x4 proj  = Matrix4x4::projection(60.0f, float(DEMO_ASPECT), 1.0f, 10000.f);

			// render
			device->BeginScene();
			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			device.setRenderTarget(cube_light_tex.getRenderTarget());
			cube_light_fx->setFloat("time", float(beat * 0.1));
			drawQuad(
				device, cube_light_fx,
				-1.0f, -1.0f,
				 2.0f, 2.0f,
				1.0f / cube_light_tex.getWidth(),
				1.0f / cube_light_tex.getHeight()
			);

			device.setRenderTarget(color_msaa.getRenderTarget());
			device.setDepthStencilSurface(depthstencil_msaa);
			D3DXCOLOR clear_color(0.0f, 0.0f, 0.0f, 1.0f);

			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			device->SetRenderState(D3DRS_ZWRITEENABLE, true);
			device->Clear(0, 0, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, clear_color, 1.f, 0);

			float fog_density = sync_get_val(fogDensityTrack, row) / 100000;

			for (int i = 0; i < 4; ++i)
				for (int j = 0; j < 4; ++j) {
					Matrix4x4 world = Matrix4x4::translation(Vector3((i - 2) * 512, 0, (j - 2) * 512));

					cube_tops_fx->setMatrices(world, view, proj);
					cube_tops_fx->setFloat("fog_density", fog_density);
					cube_tops_fx->commitChanges();
					cube_tops_fx->draw(cube_tops_x);

					cube_sides_fx->setMatrices(world, view, proj);
					cube_sides_fx->setFloat("fog_density", fog_density);
					cube_sides_fx->commitChanges();
					cube_sides_fx->draw(cube_sides_x);

					cube_floor_fx->setMatrices(world, view, proj);
					cube_floor_fx->setFloat("fog_density", fog_density);
					cube_floor_fx->commitChanges();
					cube_floor_fx->draw(cube_floor_x);
				}

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			color_msaa.resolve(device);

			world.makeIdentity();
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			device->SetRenderState(D3DRS_ZWRITEENABLE, false);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

			/* do the bloom */
			device->StretchRect(color_msaa.getSurface(0), NULL, color1_hdr.getSurface(0), NULL, D3DTEXF_LINEAR);
			RenderTexture render_textures[2] = { color1_hdr, color2_hdr };
			device->SetDepthStencilSurface(NULL);

			float stdDev = std::max(sync_get_val(bloomSizeTrack, row), 0.0f);
#if 1
			int passes = std::max((int)sync_get_val(bloomPassesTrack, row), 1);
#else
			int passes = 1;
			while (ceil(3 * stdDev) > 16) {
				passes *= 2;
				stdDev *= 0.5;
			}

			char temp[100];
			sprintf(temp, "passes: %d\n", passes);
			OutputDebugStringA(temp);
#endif

			int rtIndex = 0;
			for (int j = 0; j < 2; j++) {
				D3DXVECTOR4 gauss[8];
				float sigma = stdDev;
				float sigma_squared = sigma * sigma;

				gauss[0].x = 0.0;
				gauss[0].y = 0.0;
				gauss[0].z = 1.0f / std::max(sqrt(2.0f * 3.14159265f * sigma_squared), 1.0f);
				gauss[0].w = 0.0;

				for (int k = 1; k < 8; ++k) {
					int o1 = k * 2 - 1;
					int o2 = k * 2;

					float w1 = gauss[0].z * exp(-o1 * o1 / (2.0f * sigma_squared));
					float w2 = gauss[0].z * exp(-o2 * o2 / (2.0f * sigma_squared));

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
				}
				blur_fx->p->SetVectorArray("gauss", gauss, ARRAY_SIZE(gauss));

				for (int i = 0; i < passes; i++) {
					device.setRenderTarget(render_textures[!rtIndex].getRenderTarget(), 0);
					blur_fx->setTexture("blur_tex", render_textures[rtIndex]);
					blur_fx->commitChanges();

					drawQuad(
						device, blur_fx,
						-1.0f, -1.0f,
						 2.0f, 2.0f,
						0.5f / render_textures[rtIndex].getWidth(),
						0.5f / render_textures[rtIndex].getHeight()
					);
					rtIndex = !rtIndex;
				}
			}

			/* letterbox */
			device.setRenderTarget(backbuffer);
			device->SetDepthStencilSurface(NULL);
			device->Clear(0, 0, D3DCLEAR_TARGET, D3DXCOLOR(0, 0, 0, 0), 1.f, 0);
			device.setViewport(&letterbox_viewport);

			float flash = sync_get_val(colorMapFlashTrack, row);
			color_map_fx->setVector3("noffs", Vector3(math::notRandf(int(beat * 100)), math::notRandf(int(beat * 100) + 1), 0));
			color_map_fx->setFloat("blend", sync_get_val(colorMapBlendTrack, row));
			color_map_fx->setFloat("flash", flash < 0 ? randf() : pow(flash, 2.0f));
			color_map_fx->setFloat("fade", sync_get_val(colorMapFadeTrack, row));
			color_map_fx->setTexture("bloom", color1_hdr);
			color_map_fx->setTexture("tex", color_msaa);

			device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
			drawQuad(device, color_map_fx,
			    -1.0f, -1.0f,
			    2.0f, 2.0f,
			    0.5f / backbuffer.getWidth(),
			    0.5f / backbuffer.getHeight());
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
