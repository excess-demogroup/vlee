#pragma once

#include "resource.h"

class ConfigDialog : public CDialogImpl<ConfigDialog> {
public:
	enum { IDD = IDD_CONFIG };

	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_HANDLER(IDC_DEVICE, CBN_SELCHANGE, OnDeviceChange)
		COMMAND_HANDLER(IDC_FORMAT, CBN_SELCHANGE, OnFormatChange)
		COMMAND_HANDLER(IDC_RESOLUTION, CBN_SELCHANGE, OnResolutionChange)
		COMMAND_HANDLER(IDC_MULTISAMPLE, CBN_SELCHANGE, OnMultisampleChange)
	END_MSG_MAP()

	ConfigDialog(IDirect3D9 *direct3d);
	void reset();

	LRESULT OnInitDialog(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnDeviceChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnFormatChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnResolutionChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnMultisampleChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	UINT getAdapter() const { return adapter; }
	D3DDISPLAYMODE getMode() const { return mode; }
	UINT getWidth() const { return mode.Width; }
	UINT getHeight() const { return mode.Height; }

	D3DMULTISAMPLE_TYPE getMultisample() const { return multisample; }
	float getAspect() const { return aspect; }

	bool getVsync() const { return vsync; }
	unsigned getSoundcard() const { return soundcard; }

protected:

	void refresh_formats();
	void refresh_modes();
	void refresh_multisample_types();
	void enable_config(bool enable);

	IDirect3D9 *direct3d;
	UINT adapter;
	D3DDISPLAYMODE mode;
	D3DFORMAT format;
	D3DMULTISAMPLE_TYPE multisample;

	float aspect;

	bool vsync;
	unsigned soundcard;
};
