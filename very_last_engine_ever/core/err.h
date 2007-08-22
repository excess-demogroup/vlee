#pragma once

#include "fatalexception.h"

namespace core
{

	inline std::string d3d_get_error(HRESULT hr)
	{
		return std::string(DXGetErrorString9(hr)) + std::string(" : ") + std::string(DXGetErrorDescription9(hr));
	}

	inline void d3d_err(HRESULT hr)
	{
		// throw an error based on the errorcode
		if (FAILED(hr)) throw core::FatalException(d3d_get_error(hr));
	}

}
