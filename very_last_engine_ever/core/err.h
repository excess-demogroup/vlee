#pragma once

#include "fatalexception.h"

namespace core
{

	inline std::string d3dGetError(HRESULT hr)
	{
		return std::string(DXGetErrorString(hr)) + std::string(" : ") + std::string(DXGetErrorDescription(hr));
	}

	inline void d3dErr(HRESULT hr)
	{
		// throw an error based on the errorcode
		if (FAILED(hr)) throw core::FatalException(d3dGetError(hr));
	}

}
