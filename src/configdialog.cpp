#include "stdafx.h"
#include "configdialog.h"
#include "config.h"
#include "init.h"

#include <string.h>

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

using namespace config;

IDirect3D9 *config::direct3d = NULL;

UINT config::adapter = D3DADAPTER_DEFAULT;
D3DDISPLAYMODE config::mode =
{
	0,          // UINT Width;
	0,          // UINT Height;
	D3DPRESENT_RATE_DEFAULT, // UINT RefreshRate;
	DEFAULT_FORMAT           // D3DFORMAT Format;
};
// D3DFORMAT config::format = DEFAULT_FORMAT;
D3DMULTISAMPLE_TYPE config::multisample = D3DMULTISAMPLE_16_SAMPLES;
float config::aspect = 1.0; // float(DEFAULT_WIDTH) / DEFAULT_HEIGHT;
bool config::vsync = DEFAULT_VSYNC;
bool config::fullscreen = DEFAULT_FULLSCREEN;
unsigned config::soundcard = DEFAULT_SOUNDCARD;


static void refreshModes(HWND hDlg)
{
	int mode_count = direct3d->GetAdapterModeCount(adapter, mode.Format);
	int best_mode = 0;
	unsigned int best_mode_refresh_rate = 0;
	
	SendMessage(GetDlgItem(hDlg, IDC_RESOLUTION), (UINT)CB_RESETCONTENT, (WPARAM)0, 0);
	
	for (int i = 0; i < mode_count; ++i) {
		D3DDISPLAYMODE mode;
		direct3d->EnumAdapterModes(adapter, config::mode.Format, i, &mode);

		char temp[256];
		sprintf_s(temp, 256, "%ux%u %uhz", mode.Width, mode.Height, mode.RefreshRate);
		SendMessage(GetDlgItem(hDlg, IDC_RESOLUTION), CB_ADDSTRING, 0, (LPARAM)temp);

		if ((config::mode.Width == mode.Width) && (config::mode.Height == mode.Height)) {
			if (config::mode.RefreshRate == D3DPRESENT_RATE_DEFAULT) {
				if (best_mode_refresh_rate < mode.RefreshRate) {
					best_mode = i;
					best_mode_refresh_rate = mode.RefreshRate;
				}
			} else if (config::mode.RefreshRate == mode.RefreshRate)
				best_mode = i;
		}
	}

	SendMessage(GetDlgItem(hDlg, IDC_RESOLUTION), (UINT)CB_SETCURSEL, (WPARAM)best_mode, 0);
}

static bool is_multisample_type_ok(IDirect3D9 *direct3d, UINT adapter, D3DFORMAT backBufferFormat, D3DMULTISAMPLE_TYPE multisample_type)
{
	return
	    (multisample_type == D3DMULTISAMPLE_NONE || SUCCEEDED(direct3d->CheckDeviceFormat(adapter, D3DDEVTYPE_HAL, backBufferFormat, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F))) &&
	    SUCCEEDED(direct3d->CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, backBufferFormat, FALSE, multisample_type, NULL)) &&
	    SUCCEEDED(direct3d->CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, D3DFMT_A16B16G16R16F, FALSE, multisample_type, NULL));
}

static void refreshMultisampleTypes(HWND hDlg)
{
	SendMessage(GetDlgItem(hDlg, IDC_MULTISAMPLE), (UINT)CB_RESETCONTENT, (WPARAM)0, 0);

	struct {
		D3DMULTISAMPLE_TYPE type;
		const char *string;
	} ms_types[] = {
		{ D3DMULTISAMPLE_NONE, "no multisample" },
		{ D3DMULTISAMPLE_2_SAMPLES, "2x multisample" },
		{ D3DMULTISAMPLE_3_SAMPLES, "3x multisample" },
		{ D3DMULTISAMPLE_4_SAMPLES, "4x multisample" },
		{ D3DMULTISAMPLE_5_SAMPLES, "5x multisample" },
		{ D3DMULTISAMPLE_6_SAMPLES, "6x multisample" },
		{ D3DMULTISAMPLE_7_SAMPLES, "7x multisample" },
		{ D3DMULTISAMPLE_8_SAMPLES, "8x multisample" },
		{ D3DMULTISAMPLE_9_SAMPLES, "9x multisample" },
		{ D3DMULTISAMPLE_10_SAMPLES, "10x multisample" },
		{ D3DMULTISAMPLE_11_SAMPLES, "11x multisample" },
		{ D3DMULTISAMPLE_12_SAMPLES, "12x multisample" },
		{ D3DMULTISAMPLE_13_SAMPLES, "13x multisample" },
		{ D3DMULTISAMPLE_14_SAMPLES, "14x multisample" },
		{ D3DMULTISAMPLE_15_SAMPLES, "15x multisample" },
		{ D3DMULTISAMPLE_16_SAMPLES, "16x multisample" }
	};

	int best_hit = 0;
	int item = 0;
	for (int i = 0; i < ARRAY_SIZE(ms_types); ++i) {
		if (is_multisample_type_ok(direct3d, adapter, mode.Format, ms_types[i].type)) {
			SendMessage(GetDlgItem(hDlg, IDC_MULTISAMPLE), CB_ADDSTRING, 0, (LPARAM)ms_types[i].string);
			SendMessage(GetDlgItem(hDlg, IDC_MULTISAMPLE), CB_SETITEMDATA, item, (UINT)ms_types[i].type);
			if (config::multisample >= ms_types[i].type)
				best_hit = item;
			item++;
		}
	}

	// select previous selected mode (if found)
	SendMessage(GetDlgItem(hDlg, IDC_MULTISAMPLE), (UINT)CB_SETCURSEL, (WPARAM)best_hit, 0);
	multisample = (D3DMULTISAMPLE_TYPE)SendMessage(GetDlgItem(hDlg, IDC_MULTISAMPLE), (UINT)CB_GETITEMDATA, (WPARAM)SendMessage(GetDlgItem(hDlg, IDC_MULTISAMPLE), (UINT)CB_GETCURSEL, (WPARAM)0, 0), 0);
	EnableWindow(GetDlgItem(hDlg, IDC_MULTISAMPLE), item > 1);
}

static void refreshFormats(HWND hDlg)
{
	unsigned item = 0;

	SendMessage(GetDlgItem(hDlg, IDC_FORMAT), (UINT)CB_RESETCONTENT, (WPARAM)0, 0);

	struct {
		D3DFORMAT fmt;
		const char *str;
	} formats[] = {
		{ D3DFMT_A2R10G10B10, "A2R10G10B10" },
		{ D3DFMT_A2B10G10R10, "A2B10G10R10" },
		{ D3DFMT_A8R8G8B8, "A8R8G8B8" },
		{ D3DFMT_X8R8G8B8, "X8R8G8B8" },
		{ D3DFMT_R5G6B5, "R5G6B5" },
		{ D3DFMT_A1R5G5B5, "A1R5G5B5" },
		{ D3DFMT_X1R5G5B5, "X1R5G5B5" },
	};

	int best_hit = 0;
	for (int i = 0; i < ARRAY_SIZE(formats); ++i) {
		D3DDISPLAYMODE mode;
		if (SUCCEEDED(direct3d->EnumAdapterModes(adapter, formats[i].fmt, 0, &mode))) {
			SendMessage(GetDlgItem(hDlg, IDC_FORMAT), CB_ADDSTRING, 0, (LPARAM)formats[i].str);
			SendMessage(GetDlgItem(hDlg, IDC_FORMAT), CB_SETITEMDATA, item, formats[i].fmt);

			if (config::mode.Format == formats[i].fmt)
				best_hit = item;
			item++;
		}
	}

	SendMessage(GetDlgItem(hDlg, IDC_FORMAT), (UINT)CB_SETCURSEL, (WPARAM)0, 0);
	mode.Format = (D3DFORMAT)SendMessage(GetDlgItem(hDlg, IDC_FORMAT), (UINT)CB_GETITEMDATA, (WPARAM)best_hit, 0);
}

static void refreshAspectRatios(HWND hDlg)
{
	int best_fit = 0;
	float best_ratio = FLT_MAX;

	static const struct {
		int w, h;
	} aspect_ratios[] = {
		{5, 4},
		{4, 3},
		{16, 10},
		{16, 9},
	};

	aspect = float(mode.Width) / mode.Height;
	for (int i = 0; i < ARRAY_SIZE(aspect_ratios); ++i) {
		char temp[256];
		_snprintf(temp, 256, "%d:%d", aspect_ratios[i].w, aspect_ratios[i].h);
		SendMessage(GetDlgItem(hDlg, IDC_ASPECT), CB_ADDSTRING, 0, (LPARAM)temp);

		float curr_ratio = float(aspect_ratios[i].w) / aspect_ratios[i].h;
		if (fabs(curr_ratio - config::aspect) < fabs(best_ratio - config::aspect)) {
			best_fit = i;
			best_ratio = curr_ratio;
		}
	}
	SendMessage(GetDlgItem(hDlg, IDC_ASPECT), (UINT)CB_SETCURSEL, (WPARAM)best_fit, 0);
}

static LRESULT CALLBACK configDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR config::showDialog(HINSTANCE hInstance, IDirect3D9 *direct3d)
{
	assert(NULL != direct3d);
	config::direct3d = direct3d;
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_CONFIG), NULL,
	    (DLGPROC)configDialogProc);
}

static LRESULT onInitDialog(HWND hDlg)
{
	direct3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);

#ifndef SYNC_PLAYER
	if (direct3d->GetAdapterCount() == 1) {
		mode.Width /= 2;
		mode.Height /= 2;
	}
	aspect = float(mode.Width) / mode.Height;
	EndDialog(hDlg, IDOK);
#endif

	// add adapters to list
	unsigned adapter_count = direct3d->GetAdapterCount();
	for (unsigned i = 0; i < adapter_count; ++i) {
		D3DADAPTER_IDENTIFIER9 identifier;
		memset(&identifier, 0, sizeof(D3DADAPTER_IDENTIFIER9));
		direct3d->GetAdapterIdentifier(i, 0, &identifier);
		static char temp[256];
		sprintf_s(temp, 256, "%s on %s", identifier.DeviceName, identifier.Description);
		SendMessage(GetDlgItem(hDlg, IDC_DEVICE), CB_ADDSTRING, 0, (LPARAM)temp);
	}

	// select first adapter by default
	SendMessage(GetDlgItem(hDlg, IDC_DEVICE), (UINT)CB_SETCURSEL, (WPARAM)adapter, 0);

	refreshFormats(hDlg);
	refreshModes(hDlg);
	refreshMultisampleTypes(hDlg);
	refreshAspectRatios(hDlg);

	// set vsync checkbutton to the default setting
	CheckDlgButton(hDlg, IDC_VSYNC, DEFAULT_VSYNC);

	// set fullscreen checkbutton to the default setting
	CheckDlgButton(hDlg, IDC_FULLSCREEN, DEFAULT_FULLSCREEN);

	// playback device
	BASS_DEVICEINFO info;
	for (int i = 0; BASS_GetDeviceInfo(i, &info); ++i)
		SendMessage(GetDlgItem(hDlg, IDC_SOUNDCARD), CB_ADDSTRING, 0, (LPARAM)info.name);

	// select default soundcard
	SendMessage(GetDlgItem(hDlg, IDC_SOUNDCARD), (UINT)CB_SETCURSEL, (WPARAM)DEFAULT_SOUNDCARD, 0);

	return (LRESULT)TRUE;
}

static LRESULT onDeviceChange(HWND hDlg)
{
	adapter = (unsigned)SendMessage(GetDlgItem(hDlg, IDC_DEVICE), (UINT)CB_GETCURSEL, (WPARAM)0, 0);
	refreshFormats(hDlg);
	refreshModes(hDlg);
	refreshMultisampleTypes(hDlg);
	return (LRESULT)TRUE;
}

static LRESULT onFormatChange(HWND hDlg)
{
	mode.Format = (D3DFORMAT)SendMessage(GetDlgItem(hDlg, IDC_FORMAT), (UINT)CB_GETITEMDATA, (WPARAM)SendMessage(GetDlgItem(hDlg, IDC_FORMAT), (UINT)CB_GETCURSEL, (WPARAM)0, 0), 0);
	refreshModes(hDlg);
	refreshMultisampleTypes(hDlg);
	return (LRESULT)TRUE;
}

static LRESULT onResolutionChange(HWND hDlg)
{
	direct3d->EnumAdapterModes(adapter, mode.Format, (UINT)SendMessage(GetDlgItem(hDlg, IDC_RESOLUTION), (UINT)CB_GETCURSEL, (WPARAM)0, 0), &mode);
	refreshMultisampleTypes(hDlg);
	refreshAspectRatios(hDlg);
	return (LRESULT)TRUE;
}

static LRESULT onMultisampleChange(HWND hDlg)
{
	multisample = (D3DMULTISAMPLE_TYPE)SendMessage(GetDlgItem(hDlg, IDC_MULTISAMPLE), (UINT)CB_GETITEMDATA, (WPARAM)SendMessage(GetDlgItem(hDlg, IDC_MULTISAMPLE), (UINT)CB_GETCURSEL, (WPARAM)0, 0), 0);
	return 0;
}

static LRESULT onCloseCmd(HWND hDlg, WORD wID)
{
	if (IDOK == wID) {
		vsync = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_VSYNC));
		fullscreen = (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_FULLSCREEN));
		soundcard = (unsigned)SendMessage(GetDlgItem(hDlg, IDC_SOUNDCARD), (UINT)CB_GETCURSEL, (WPARAM)0, 0);

		char temp[256];
		int sel = (unsigned)SendMessage(GetDlgItem(hDlg, IDC_ASPECT), (UINT)CB_GETCURSEL, (WPARAM)0, 0);

		SendMessage(GetDlgItem(hDlg, IDC_ASPECT), (UINT)CB_GETLBTEXT, (WPARAM)sel, (LPARAM)temp);
		int w, h;
		sscanf(temp, "%d:%d", &w, &h);
		aspect = float(w) / h;
		sprintf(temp, "test %d:%d, %f\n", w, h, aspect);
	}
	EndDialog(hDlg, wID);
	return 0;
}

static LRESULT CALLBACK configDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG:
		return onInitDialog(hDlg);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			return onCloseCmd(hDlg, LOWORD(wParam));
			break;

		case IDC_DEVICE:
			if (CBN_SELCHANGE == HIWORD(wParam))
				return onDeviceChange(hDlg);
			break;

		case IDC_FORMAT:
			if (CBN_SELCHANGE == HIWORD(wParam))
				return onFormatChange(hDlg);
			break;

		case IDC_RESOLUTION:
			if (CBN_SELCHANGE == HIWORD(wParam))
				return onResolutionChange(hDlg);
			break;

		case IDC_MULTISAMPLE:
			if (CBN_SELCHANGE == HIWORD(wParam))
				return onMultisampleChange(hDlg);
			break;
		}
	}

	return FALSE;
}
