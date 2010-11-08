#pragma once

#include "core/err.h"
#include "core/comref.h"
#include "renderer/device.h"
#include "renderer/indexbuffer.h"

#include "engine/drawable.h"

namespace engine
{
	class Mesh : public ComRef<ID3DXMesh>, public Drawable
	{
	public:
		Mesh() : ComRef<ID3DXMesh>() {}
		Mesh(ID3DXMesh *mesh) : ComRef<ID3DXMesh>(mesh) {}
		
		void draw()
		{
			p->DrawSubset(0);
		}

		void draw(UINT subset)
		{
			p->DrawSubset(subset);
		}

		renderer::IndexBuffer getIndexBuffer()
		{
			LPDIRECT3DINDEXBUFFER9 indexBuffer;
			core::d3dErr(p->GetIndexBuffer(&indexBuffer));
			return renderer::IndexBuffer(indexBuffer);
		}
	};

	inline Mesh *loadMesh(renderer::Device &device, std::string filename)
	{
		ID3DXMesh *mesh = NULL;
		HRESULT hr = D3DXLoadMeshFromX(filename.c_str(), 0, device, 0, 0, 0, 0, &mesh);
		if (FAILED(hr)) throw core::FatalException(std::string("failed to load mesh \"") + filename + std::string("\"\n\n") + core::d3dGetError(hr));
		assert(NULL != mesh);
		
		Mesh *mesh_wrapper = new Mesh;
		mesh_wrapper->attachRef(mesh);
		return mesh_wrapper;
	}
}
