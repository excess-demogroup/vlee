#include "stdafx.h"
#include "configdialog.h"
#include "config.h"
#include "init.h"

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))

ConfigDialog::ConfigDialog(IDirect3D9 *direct3d) : direct3d(direct3d)
{
	assert(0 != direct3d);
	reset();
}

void ConfigDialog::reset()
{
	mode.Width = DEFAULT_WIDTH;
	mode.Height = DEFAULT_HEIGHT;
	aspect = float(mode.Width) / mode.Height;

	mode.Format = DEFAULT_FORMAT;
	mode.RefreshRate = D3DPRESENT_RATE_DEFAULT;
	vsync = DEFAULT_VSYNC;
	adapter = D3DADAPTER_DEFAULT;
	multisample = DEFAULT_MULTISAMPLE;
	soundcard = DEFAULT_SOUNDCARD;
}

LRESULT ConfigDialog::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// add adapters to list
	unsigned adapter_count = direct3d->GetAdapterCount();
	for (unsigned i = 0; i < adapter_count; ++i) {
		D3DADAPTER_IDENTIFIER9 identifier;
		memset(&identifier, 0, sizeof(D3DADAPTER_IDENTIFIER9));
		direct3d->GetAdapterIdentifier(i, 0, &identifier);
		static char temp[256];
		sprintf_s(temp, 256, "%s on %s", identifier.DeviceName, identifier.Description);
		SendMessage(GetDlgItem(IDC_DEVICE), CB_ADDSTRING, 0, (LPARAM)temp);
	}

	// select first adapter by default
	SendMessage(GetDlgItem(IDC_DEVICE), (UINT)CB_SETCURSEL, (WPARAM)adapter, 0);

	refresh_formats();
	refresh_modes();
	refresh_multisample_types();

	// set vsync checkbutton to the default setting
	CheckDlgButton(IDC_VSYNC, DEFAULT_VSYNC);

	// setup aspect ratio 
	SendMessage(GetDlgItem(IDC_ASPECT), CB_ADDSTRING, 0, (LPARAM)"4:3");
	SendMessage(GetDlgItem(IDC_ASPECT), CB_ADDSTRING, 0, (LPARAM)"16:10");
	SendMessage(GetDlgItem(IDC_ASPECT), CB_ADDSTRING, 0, (LPARAM)"16:9");

	float aspect = float(GetSystemMetrics(SM_CXSCREEN)) / GetSystemMetrics(SM_CYSCREEN);
	if      (fabs(aspect - (16.f /  9)) < 1e-5) SendMessage(GetDlgItem(IDC_ASPECT), (UINT)CB_SETCURSEL, (WPARAM)2, 0);
	else if (fabs(aspect - (16.f / 10)) < 1e-5) SendMessage(GetDlgItem(IDC_ASPECT), (UINT)CB_SETCURSEL, (WPARAM)1, 0);
	else                                        SendMessage(GetDlgItem(IDC_ASPECT), (UINT)CB_SETCURSEL, (WPARAM)0, 0);

	// select medium by default
//	CheckDlgButton(IDC_MEDIUM, true);
//	SendMessage(GetDlgItem(IDC_MEDIUM), (UINT)BN_CLICKED, (WPARAM)1, 0);
//	::PostMessage(GetDlgItem(IDC_MEDIUM), (UINT)BN_CLICKED, (WPARAM)0, 0);

#ifndef VJSYS
	// playback device
	for (unsigned i = 0; 0 != BASS_GetDeviceDescription(i); ++i) {
		SendMessage(GetDlgItem(IDC_SOUNDCARD), CB_ADDSTRING, 0, (LPARAM)BASS_GetDeviceDescription(i));
	}
#else
	// record device
	for (unsigned i = 0; 0 != BASS_RecordGetDeviceDescription(i); ++i) {
		SendMessage(GetDlgItem(IDC_SOUNDCARD), CB_ADDSTRING, 0, (LPARAM)BASS_RecordGetDeviceDescription(i));
	}
#endif

	// select default soundcard
	SendMessage(GetDlgItem(IDC_SOUNDCARD), (UINT)CB_SETCURSEL, (WPARAM)DEFAULT_SOUNDCARD, 0);

	return (LRESULT)TRUE;
}

LRESULT ConfigDialog::OnDeviceChange(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	adapter = (unsigned)SendMessage(GetDlgItem(IDC_DEVICE), (UINT)CB_GETCURSEL, (WPARAM)0, 0);
	refresh_formats();
	refresh_modes();
	refresh_multisample_types();
	return (LRESULT)TRUE;
}

LRESULT ConfigDialog::OnFormatChange(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	format = (D3DFORMAT)SendMessage(GetDlgItem(IDC_FORMAT), (UINT)CB_GETITEMDATA, (WPARAM)SendMessage(GetDlgItem(IDC_FORMAT), (UINT)CB_GETCURSEL, (WPARAM)0, 0), 0);
	refresh_modes();
	refresh_multisample_types();
	return (LRESULT)TRUE;
}

LRESULT ConfigDialog::OnResolutionChange(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	direct3d->EnumAdapterModes(adapter, format, (UINT)SendMessage(GetDlgItem(IDC_RESOLUTION), (UINT)CB_GETCURSEL, (WPARAM)0, 0), &mode);
	refresh_multisample_types();
	return (LRESULT)TRUE;
}

LRESULT ConfigDialog::OnMultisampleChange(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	multisample = (D3DMULTISAMPLE_TYPE)SendMessage(GetDlgItem(IDC_MULTISAMPLE), (UINT)CB_GETITEMDATA, (WPARAM)SendMessage(GetDlgItem(IDC_MULTISAMPLE), (UINT)CB_GETCURSEL, (WPARAM)0, 0), 0);
		// throw FatalException("mordi");
	return 0;
}

LRESULT ConfigDialog::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (IDOK == wID) {
		vsync = (BST_CHECKED == IsDlgButtonChecked(IDC_VSYNC));
		soundcard = (unsigned)SendMessage(GetDlgItem(IDC_SOUNDCARD), (UINT)CB_GETCURSEL, (WPARAM)0, 0);

		char temp[256];
		int sel = (unsigned)SendMessage(GetDlgItem(IDC_ASPECT), (UINT)CB_GETCURSEL, (WPARAM)0, 0);

		SendMessage(GetDlgItem(IDC_ASPECT), (UINT)CB_GETLBTEXT, (WPARAM)sel, (LPARAM)temp);
		int w, h;
		sscanf(temp, "%d:%d", &w, &h);
		aspect = float(w) / h;
		sprintf(temp, "test %d:%d, %f\n", w, h, aspect);
	}
	EndDialog(wID);
	return 0;
}

void ConfigDialog::refresh_modes() {
	unsigned mode_count = direct3d->GetAdapterModeCount(adapter, format);
	unsigned best_mode = 0;
	unsigned best_mode_refresh_rate = 0;

	SendMessage(GetDlgItem(IDC_RESOLUTION), (UINT)CB_RESETCONTENT, (WPARAM)0, 0);

	for (unsigned i = 0; i < mode_count; ++i) {
		D3DDISPLAYMODE mode;
		direct3d->EnumAdapterModes(adapter, format, i, &mode);

		char temp[256];
		sprintf_s(temp, 256, "%ux%u %uhz", mode.Width, mode.Height, mode.RefreshRate);
		SendMessage(GetDlgItem(IDC_RESOLUTION), CB_ADDSTRING, 0, (LPARAM)temp);

		if ((this->mode.Width == mode.Width) && (this->mode.Height == mode.Height)) {
			if (this->mode.RefreshRate == D3DPRESENT_RATE_DEFAULT) {
				if (best_mode_refresh_rate < mode.RefreshRate) {
					best_mode = i;
					best_mode_refresh_rate = mode.RefreshRate;
				}
			} else if (this->mode.RefreshRate == mode.RefreshRate) {
					best_mode = i;
			}
		}
	}

	SendMessage(GetDlgItem(IDC_RESOLUTION), (UINT)CB_SETCURSEL, (WPARAM)best_mode, 0);
}

static bool is_multisample_type_ok(IDirect3D9 *direct3d, UINT Adapter, D3DFORMAT DepthBufferFormat, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, D3DMULTISAMPLE_TYPE multisample_type) {
	if (SUCCEEDED(direct3d->CheckDeviceMultiSampleType(Adapter, D3DDEVTYPE_HAL, BackBufferFormat, FALSE, multisample_type, NULL)) &&
	    SUCCEEDED(direct3d->CheckDeviceMultiSampleType(Adapter, D3DDEVTYPE_HAL, DepthBufferFormat, FALSE, multisample_type, NULL)))
	{
		return true;
	}
	return false;
}

void ConfigDialog::refresh_multisample_types() {
	SendMessage(GetDlgItem(IDC_MULTISAMPLE), (UINT)CB_RESETCONTENT, (WPARAM)0, 0);

	static const D3DMULTISAMPLE_TYPE types[] = { D3DMULTISAMPLE_NONE, /* D3DMULTISAMPLE_NONMASKABLE, */ D3DMULTISAMPLE_2_SAMPLES, D3DMULTISAMPLE_3_SAMPLES, D3DMULTISAMPLE_4_SAMPLES, D3DMULTISAMPLE_5_SAMPLES, D3DMULTISAMPLE_6_SAMPLES, D3DMULTISAMPLE_7_SAMPLES, D3DMULTISAMPLE_8_SAMPLES, D3DMULTISAMPLE_9_SAMPLES, D3DMULTISAMPLE_10_SAMPLES, D3DMULTISAMPLE_11_SAMPLES, D3DMULTISAMPLE_12_SAMPLES, D3DMULTISAMPLE_13_SAMPLES, D3DMULTISAMPLE_14_SAMPLES, D3DMULTISAMPLE_15_SAMPLES, D3DMULTISAMPLE_16_SAMPLES };
//	static const char *type_strings[] = { "MULTISAMPLE NONE", /* "MULTISAMPLE NONMASKABLE", */ "MULTISAMPLE 2 SAMPLES", "MULTISAMPLE 3 SAMPLES", "MULTISAMPLE 4 SAMPLES", "MULTISAMPLE 5 SAMPLES", "MULTISAMPLE 6 SAMPLES", "MULTISAMPLE 7 SAMPLES", "MULTISAMPLE 8 SAMPLES", "MULTISAMPLE 9 SAMPLES", "MULTISAMPLE 10 SAMPLES", "MULTISAMPLE 11 SAMPLES", "MULTISAMPLE 12 SAMPLES", "MULTISAMPLE 13 SAMPLES", "MULTISAMPLE 14 SAMPLES", "MULTISAMPLE 15 SAMPLES", "MULTISAMPLE 16 SAMPLES" };
	static const char *type_strings[] = { "no multisample", /* "MULTISAMPLE NONMASKABLE", */ "2x multisample", "3x multisample", "4x multisample", "5x multisample", "6x multisample", "7x multisample", "8x multisample", "9x multisample", "10x multisample", "11x multisample", "12x multisample", "13x multisample", "14x multisample", "15x multisample", "16x multisample" };
	assert(ARRAY_SIZE(types) == ARRAY_SIZE(type_strings));

	unsigned best_hit = 0;
	unsigned item = 0;
	for (unsigned i = 0; i < ARRAY_SIZE(types); ++i) {
		if (true == is_multisample_type_ok(direct3d, adapter, format, format, get_best_depth_stencil_format(direct3d, adapter, format), types[i]))
		{
			SendMessage(GetDlgItem(IDC_MULTISAMPLE), CB_ADDSTRING, 0, (LPARAM)type_strings[i]);
			SendMessage(GetDlgItem(IDC_MULTISAMPLE), CB_SETITEMDATA, item, (UINT)types[i]);
			if (this->multisample == types[i]) best_hit = item;
			item++;
		}
	}

	// select previous selected mode (if found)
	SendMessage(GetDlgItem(IDC_MULTISAMPLE), (UINT)CB_SETCURSEL, (WPARAM)best_hit, 0);
}

void ConfigDialog::refresh_formats() {
	unsigned item = 0;

	SendMessage(GetDlgItem(IDC_FORMAT), (UINT)CB_RESETCONTENT, (WPARAM)0, 0);  

	static const D3DFORMAT formats[] = { D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_A2R10G10B10, D3DFMT_R5G6B5, D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5 };
	static const char *format_strings[] = { "A8R8G8B8", "X8R8G8B8", "A2R10G10B10", "R5G6B5", "X1R5G5B5", "A1R5G5B5" };
	assert(ARRAY_SIZE(formats) == ARRAY_SIZE(format_strings));

	unsigned best_hit = 0;

	for (unsigned i = 0; i < (sizeof(formats) / sizeof(formats[0])); ++i) {
		D3DDISPLAYMODE mode;
		if (SUCCEEDED(direct3d->EnumAdapterModes(adapter, formats[i], 0, &mode))) {
			SendMessage(GetDlgItem(IDC_FORMAT), CB_ADDSTRING, 0, (LPARAM)format_strings[i]);
			SendMessage(GetDlgItem(IDC_FORMAT), CB_SETITEMDATA, item, formats[i]);

			if (this->format == formats[i]) best_hit = item;
			item++;
		}
	}

	SendMessage(GetDlgItem(IDC_FORMAT), (UINT)CB_SETCURSEL, (WPARAM)0, 0);
	format = (D3DFORMAT)SendMessage(GetDlgItem(IDC_FORMAT), (UINT)CB_GETITEMDATA, (WPARAM)best_hit, 0);
}

void ConfigDialog::enable_config(bool enable) {
//	::ShowWindow(GetDlgItem(IDC_FORMAT), SW_HIDE);
	::EnableWindow(GetDlgItem(IDC_DEVICE), enable);
	::EnableWindow(GetDlgItem(IDC_FORMAT), enable);
	::EnableWindow(GetDlgItem(IDC_RESOLUTION), enable);
	::EnableWindow(GetDlgItem(IDC_MULTISAMPLE), enable);
	::EnableWindow(GetDlgItem(IDC_VSYNC), enable);
	::EnableWindow(GetDlgItem(IDC_SOUNDCARD), enable);
}
