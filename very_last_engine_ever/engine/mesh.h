#pragma once

#include "core/err.h"
#include "renderer/device.h"

#include "engine/drawable.h"

namespace engine
{
	class Mesh : public CComPtr<ID3DXMesh>, public Drawable
	{
		void draw()
		{
			p->DrawSubset(0);
		}
	};

	inline Mesh loadMesh(renderer::Device &device, std::string filename)
	{
		Mesh mesh;
		HRESULT hr = D3DXLoadMeshFromX(filename.c_str(), 0, device, 0, 0, 0, 0, &mesh);
		if (FAILED(hr)) throw core::FatalException(std::string("failed to load mesh \"") + filename + std::string("\"\n\n") + core::d3d_get_error(hr));
		return mesh;
	}
}
