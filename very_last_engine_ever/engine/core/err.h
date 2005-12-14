#pragma once

inline void d3d_err(HRESULT hr)
{
	// throw an error based on the errorcode
	if (FAILED(hr)) throw FatalException(std::string(DXGetErrorString9(hr)) + std::string(" : ") + std::string(DXGetErrorDescription9(hr)));
}
