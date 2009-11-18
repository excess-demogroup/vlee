#pragma once

#include "core/err.h"
#include "renderer/device.h"
#include "renderer/indexbuffer.h"

#include "engine/drawable.h"

namespace engine
{
	class Mesh : public CComPtr<ID3DXMesh>, public Drawable
	{
	public:
		Mesh() : CComPtr<ID3DXMesh>() {}
		Mesh(ID3DXMesh *mesh) : CComPtr<ID3DXMesh>(mesh) {}
		
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

		void computeBinormalTangents()
		{
#if 0
			ID3DXMesh *result;
			core::d3dErr(
				D3DXComputeTangentFrameEx(
					p,
					D3DX_DEFAULT, 0,
					D3DDECLUSAGE_TANGENT, 0,
					D3DDECLUSAGE_BINORMAL, 0,
					D3DDECLUSAGE_NORMAL, 0,
					D3DXTANGENT_CALCULATE_NORMALS,
					CONST DWORD * pdwAdjacency,
					FLOAT fPartialEdgeThreshold,
					FLOAT fSingularPointThreshold,
					FLOAT fNormalEdgeThreshold,
					&result,
					NULL
				)
			);
			
			p = result;
#endif
		}
	};

	inline Mesh *loadMesh(renderer::Device &device, std::string filename)
	{
		ID3DXMesh *mesh = NULL;
		HRESULT hr = D3DXLoadMeshFromX(filename.c_str(), 0, device, 0, 0, 0, 0, &mesh);
		if (FAILED(hr)) throw core::FatalException(std::string("failed to load mesh \"") + filename + std::string("\"\n\n") + core::d3dGetError(hr));
		assert(NULL != mesh);
		
		Mesh *mesh_wrapper = new Mesh;
		mesh_wrapper->Attach(mesh);
		return mesh_wrapper;
	}
}
