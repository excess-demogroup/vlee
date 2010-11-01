#include "stdafx.h"
#include "effect.h"

#include "../renderer/device.h"
#include "../core/fatalexception.h"
#include "../core/err.h"

using renderer::Device;

namespace engine
{
	Effect *loadEffect(Device &device, std::string filename)
	{
		ID3DXEffect *eff = NULL;
		ID3DXBuffer *err_buf = NULL;

		DWORD flags = 0; // D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;
#ifdef _RELEASE
		flags |= D3DXSHADER_SKIPVALIDATION;
#endif

		HRESULT hr = D3DXCreateEffectFromFile(device, filename.c_str(), NULL, NULL, 0, NULL, &eff, &err_buf);
		
		if (FAILED(hr)) {
			if (NULL == err_buf)
				throw core::FatalException(std::string("failed to load effect \"") + filename + std::string("\"\n\n") + core::d3dGetError(hr));
			throw core::FatalException(std::string("failed to load effect \"") + filename + std::string("\"\n\n") + std::string((const char*)err_buf->GetBufferPointer()));
		}

		Effect *eff_wrapper = new Effect;
		eff_wrapper->attachRef(eff);
		eff_wrapper->update();
		return eff_wrapper;
	}
}
