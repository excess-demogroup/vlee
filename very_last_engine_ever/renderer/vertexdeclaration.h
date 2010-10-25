#pragma once

#include "../core/fatalexception.h"
#include "../core/comref.h"

namespace renderer
{
	class VertexDeclaration : public ComRef<IDirect3DVertexDeclaration9>
	{
	public:
		VertexDeclaration(IDirect3DVertexDeclaration9 *decl = NULL)
			: ComRef<IDirect3DVertexDeclaration9>(decl)
		{
		}
	};
}
