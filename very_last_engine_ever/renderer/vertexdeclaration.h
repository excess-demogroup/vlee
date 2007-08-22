#pragma once

#include "../core/fatalexception.h"

namespace renderer
{
	class VertexDeclaration : public CComPtr<IDirect3DVertexDeclaration9>
	{
	public:
		VertexDeclaration(IDirect3DVertexDeclaration9 *decl = NULL)
			: CComPtr<IDirect3DVertexDeclaration9>(decl)
		{
		}
	};
}
