#pragma once

#include "../core/fatalexception.h"

namespace renderer
{
	class VertexDeclaration : public CComPtr<IDirect3DVertexDeclaration9>
	{
	public:
		VertexDeclaration() : CComPtr<IDirect3DVertexDeclaration9>() {}

		VertexDeclaration(Device &device, CONST D3DVERTEXELEMENT9* vertex_elements)
		{
			assert(0 != device);
			core::log::printf("creating vertexdeclaration... ");
			IDirect3DVertexDeclaration9 *decl;
			if (FAILED(device->CreateVertexDeclaration(vertex_elements, &decl)))
				throw core::FatalException("failed to create vertex declaration");
			core::log::printf("done.\n");
			Attach(decl); // don't addref
		}
	private:
		IDirect3DVertexDeclaration9* decl;
	};
}
