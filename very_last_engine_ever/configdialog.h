#pragma once

#include "resource.h"

namespace config
{
	INT_PTR showDialog(HINSTANCE hInstance, IDirect3D9 *direct3d);
	
	LRESULT OnInitDialog(UINT Msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnDeviceChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnFormatChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnResolutionChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnMultisampleChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

/*	UINT adapter;

	void refreshFormats();
	void refreshModes();
	void refreshMultisampleTypes();
	void enableConfig(bool enable);
*/
	extern IDirect3D9 *direct3d;
	extern UINT adapter;
	extern D3DDISPLAYMODE mode;
//	extern D3DFORMAT format;
	extern D3DMULTISAMPLE_TYPE multisample;
	extern float aspect;
	extern bool vsync;
	extern unsigned soundcard;
};
