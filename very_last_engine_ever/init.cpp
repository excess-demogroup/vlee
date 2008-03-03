#include "stdafx.h"
#include "config.h"
#include "init.h"

#include "core/fatalexception.h"
using core::FatalException;

static bool is_depth_format_ok(IDirect3D9 *direct3d, UINT Adapter, D3DFORMAT DepthFormat, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat)
{
	// Verify that the depth format exists
	HRESULT hr = direct3d->CheckDeviceFormat(Adapter, DEVTYPE, AdapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, DepthFormat);
	if (FAILED(hr)) return false;

	// Verify that the depth format is compatible
	hr = direct3d->CheckDepthStencilMatch(Adapter, DEVTYPE, AdapterFormat, BackBufferFormat, DepthFormat);
	return SUCCEEDED(hr);
}

D3DFORMAT init::get_best_depth_stencil_format(IDirect3D9 *direct3d, UINT adapter, D3DFORMAT format)
{
	// a list of supported depth/stencil formats
#if (!NEED_STENCIL)
	static const D3DFORMAT formats[] = { D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D24X8, D3DFMT_D32, D3DFMT_D16, D3DFMT_D15S1 };
#else
	static const D3DFORMAT formats[] = { D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D15S1 };
#endif
	// loop through the list and check the formats
	for (unsigned i = 0; i < (sizeof(formats) / sizeof(formats[0])); ++i)
	{
		// i'm not really sure what the difference between adapter-format and backbuffer-format is...
		if (is_depth_format_ok(direct3d, adapter, formats[i], format, format)) return formats[i];
	}
	throw FatalException("no proper depth/stencil format found.\ntry another device-format.");
}


void print_GUID(GUID g)
{
	printf("const GUID GUID_name = { 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x };\n", g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
}

static const GUID kusma_laptop_bug = { 0xd7b71e3e, 0x4235, 0x11cf, 0x65, 0x6b, 0x18, 0x20, 0x2, 0xc2, 0xcb, 0x35 };

IDirect3DDevice9 *init::initD3D(IDirect3D9 *direct3d, HWND win, D3DDISPLAYMODE mode, D3DMULTISAMPLE_TYPE multisample, unsigned adapter, bool vsync) {
	D3DCAPS9 caps;
	direct3d->GetDeviceCaps(adapter, DEVTYPE, &caps);
	DWORD tnl = D3DCREATE_HARDWARE_VERTEXPROCESSING;

	if (!((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) && (caps.VertexShaderVersion >= MIN_VS_VERSION))) {
		tnl = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
		MessageBox(NULL, "your gpu is still from poland!", "performance warning", MB_OK | MB_ICONWARNING);
	}

	UINT present_interval = D3DPRESENT_INTERVAL_DEFAULT;
	if (!vsync) present_interval = D3DPRESENT_INTERVAL_IMMEDIATE;

	D3DPRESENT_PARAMETERS present_parameters;
	ZeroMemory(&present_parameters, sizeof(D3DPRESENT_PARAMETERS));
	present_parameters.BackBufferWidth = mode.Width;
	present_parameters.BackBufferHeight = mode.Height;
	present_parameters.BackBufferFormat = mode.Format;
	present_parameters.BackBufferFormat = mode.Format;
	present_parameters.BackBufferCount = 1;
//	present_parameters.MultiSampleType = D3DMULTISAMPLE_NONE;
	present_parameters.MultiSampleType = multisample;
//	present_parameters.MultiSampleType = D3DMULTISAMPLE_6_SAMPLES;
	present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	present_parameters.Windowed = WINDOWED;
	present_parameters.EnableAutoDepthStencil = TRUE;
	present_parameters.AutoDepthStencilFormat = init::get_best_depth_stencil_format(direct3d, adapter, mode.Format);
	present_parameters.PresentationInterval = present_interval;
//	present_parameters.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;

	D3DADAPTER_IDENTIFIER9 identifier;
	memset(&identifier, 0, sizeof(D3DADAPTER_IDENTIFIER9));
	direct3d->GetAdapterIdentifier(adapter, 0, &identifier);

//	print_GUID(identifier.DeviceIdentifier);
	if (kusma_laptop_bug == identifier.DeviceIdentifier)
	{
#ifdef NDEBUG
		MessageBox(NULL, "It appears you are using a very sketchy combination\nof hardware and drivers. Due to a driver-bug,\ntripple buffering will be disabled.", "performance warning", MB_OK | MB_ICONWARNING);
#endif
		present_parameters.BackBufferCount = 1;
	}


#if WINDOWED
	present_parameters.FullScreen_RefreshRateInHz = 0;
#else
	present_parameters.FullScreen_RefreshRateInHz = mode.RefreshRate;
#endif

	D3DDEVTYPE devtype = DEVTYPE;

	D3DADAPTER_IDENTIFIER9 Identifier;
	direct3d->GetAdapterIdentifier(adapter,0,&Identifier);
	if (strstr(Identifier.Description,"PerfHUD") != 0)
	{
		devtype = D3DDEVTYPE_REF;
	}

	IDirect3DDevice9 *device;
	HRESULT result = direct3d->CreateDevice(adapter, devtype, win, tnl, &present_parameters, &device);

	if (FAILED(result))
	{
		// in case the backbuffer-count was too high, try again (it should be turned down to max by the driver)
		result = direct3d->CreateDevice(adapter, devtype, win, tnl, &present_parameters, &device);
		if (FAILED(result))
		{
			throw FatalException(std::string(DXGetErrorString9(result)) + std::string(" : ") + std::string(DXGetErrorDescription9(result)));
		}
	}

	assert(0 != device);
	return device;
}
