#pragma once

#include "core/err.h"

class Mesh : public CComPtr<ID3DXMesh>, public Drawable
{
	void draw()
	{
		p->DrawSubset(0);
	}
};

inline Mesh load_mesh(engine::core::Device &device, std::string filename)
{
	Mesh mesh;
	HRESULT hr = D3DXLoadMeshFromX("data/test.x", 0, device, 0, 0, 0, 0, &mesh);
	if (FAILED(hr)) throw core::FatalException(std::string("failed to load mesh \"") + filename + std::string("\"\n\n") + core::d3d_get_error(hr));
	return mesh;
}
