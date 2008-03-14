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
		HRESULT hr = D3DXCreateEffectFromFile(device, filename.c_str(), NULL, NULL, 0, NULL, &eff, &err_buf);
		
		if (FAILED(hr))
		{
			if (NULL == err_buf) throw core::FatalException(std::string("failed to load mesh \"") + filename + std::string("\"\n\n") + core::d3dGetError(hr));
			throw core::FatalException(std::string("failed to load mesh \"") + filename + std::string("\"\n\n") + std::string((const char*)err_buf->GetBufferPointer()));
		}

		Effect *eff_wrapper = new Effect;
		eff_wrapper->Attach(eff);
		eff_wrapper->update();
		return eff_wrapper;
	}
}
