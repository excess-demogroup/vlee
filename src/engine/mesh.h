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

		int getVertexCount() const
		{
			return p->GetNumVertices();
		}

		D3DVERTEXELEMENT9 getVertexElementFromUsage(D3DDECLUSAGE usage, const int index = 0) const
		{
			D3DVERTEXELEMENT9 vertex_decl[MAX_FVF_DECL_SIZE];
			p->GetDeclaration(vertex_decl);
			for (int i = 0; i < MAX_FVF_DECL_SIZE; ++i)
				if (vertex_decl[i].Usage == usage && vertex_decl[i].UsageIndex == index)
					return vertex_decl[i];
			throw core::FatalException("vertex element not found!");
		}

		void getVertexPositions(math::Vector3 *dst, int start, int end)
		{
			D3DVERTEXELEMENT9 element = getVertexElementFromUsage(D3DDECLUSAGE_POSITION);
			assert(element.Type == D3DDECLTYPE_FLOAT3);
			void *data;
			core::d3dErr(p->LockVertexBuffer(D3DLOCK_READONLY, &data));
			int stride = p->GetNumBytesPerVertex();
			for (int i = start; i < end; ++i)
				memcpy(dst + i, (unsigned char *)data + element.Offset + i * stride, sizeof(float) * 3);
			p->UnlockVertexBuffer();
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
		if (FAILED(hr))
			throw core::FatalException(std::string("failed to load mesh \"") + filename + std::string("\"\n\n") + core::d3dGetError(hr));
		assert(NULL != mesh);

		Mesh *mesh_wrapper = new Mesh;
		mesh_wrapper->attachRef(mesh);
		return mesh_wrapper;
	}
}
