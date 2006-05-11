#include "stdafx.h"
#include "resource.h"
#include "config.h"
#include "configdialog.h"
#include "init.h"
#include "engine/engine.h"

WTL::CAppModule _Module;

using engine::core::FatalException;
using engine::core::Surface;
using engine::core::d3d_err;
using namespace engine::core;

using engine::Mesh;
using engine::Effect;

// using namespace engine::core;
using namespace engine::scenegraph;

HWND win = 0;
HSTREAM stream = 0;

static float spectrum[256];

float randf()
{
	return rand() * (1.f / RAND_MAX);
}

template <typename T>
CComPtr<T> d3d_ptr(T *ptr) {
	// make a CComPtr<T> without adding a reference
	CComPtr<T> com_ptr;
	com_ptr.Attach(ptr); // don't addref
	return com_ptr;
}

#include "megademo.h"

int main(int /*argc*/, char* /*argv*/ [])
{

#ifndef NDEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
//	_CrtSetBreakAlloc(68);
#endif

	_Module.Init(NULL, GetModuleHandle(0));

	try
	{
		CComPtr<IDirect3D9> direct3d = d3d_ptr(Direct3DCreate9(D3D_SDK_VERSION));
		if (!direct3d) throw FatalException("your directx-version is from the stone-age.\n\nTHRUG SAYS: UPGRADE!");

		ConfigDialog config(direct3d);

#ifdef NDEBUG
		INT_PTR result = config.DoModal();
		if (FAILED(result)) MessageBox(NULL, "could not initialize dialogbox, using default settings", NULL, MB_OK);
		else
		{
			if (IDOK != result)
			{
				// cancel was hit...
				MessageBox(NULL, "damn whimp...", "pfff", MB_OK);
				return 0;
			}
		}
#endif

		D3DDISPLAYMODE mode = config.get_mode();
		win = CreateWindow("static", "very last engine ever", WS_POPUP, 0, 0, mode.Width, mode.Height, 0, 0, GetModuleHandle(0), 0);
		if (!win) throw FatalException("CreateWindow() failed. something is VERY spooky.");

		Device device;
		device.Attach(init_d3d(direct3d, win, mode, config.get_multisample(), config.get_adapter(), config.get_vsync()));

		ShowWindow(win, TRUE); // showing window after initing d3d in order to be able to see warnings during init

#ifndef VJSYS
		if (!BASS_Init(config.get_soundcard(), 44100, BASS_DEVICE_LATENCY, 0, 0)) throw FatalException("failed to init bass");
		stream = BASS_StreamCreateFile(false, "data/irvin_-_phyllis_still_craves_for_it.ogg", 0, 0, BASS_MP3_SETPOS | ((0 == config.get_soundcard()) ? BASS_STREAM_DECODE : 0));
		if (!stream) throw FatalException("failed to open tune");
#else
		BASS_RecordInit(config.get_soundcard());
		HRECORD stream = BASS_RecordStart(44100, 2, BASS_RECORD_PAUSE, 0, 0);
		if (!stream) throw FatalException("failed to open input-device");
#endif


#if !WINDOWED
		ShowCursor(0);
#endif

		MegaDemo demo(device);
		demo.start();

		BASS_Start();
		BASS_ChannelPlay(stream, false);
//		BASS_ChannelSetPosition(stream, BASS_ChannelSeconds2Bytes(stream, 60.0f) + 10);

		bool done = false;
		while (!done)
		{

			static float last_time = 0.f;
			static float time_offset = 0.f;
			float time = BASS_ChannelBytes2Seconds(stream, BASS_ChannelGetPosition(stream));

			demo.draw(time);

#ifndef NDEBUG
//			printf("real_time: %f\n", real_time);
#endif

			BASS_Update(); // decrease the chance of missing vsync

			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				if (WM_QUIT == msg.message) done = true;
				if (WM_KEYDOWN == msg.message)
				{
					if (VK_ESCAPE == LOWORD(msg.wParam)) done = true;
				}
			}
		}

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
