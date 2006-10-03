#pragma once

namespace core
{
	class VertexDeclaration {
	public:
		VertexDeclaration(IDirect3DVertexDeclaration9* decl) : decl(decl) {}

		VertexDeclaration(IDirect3DDevice9 *device, CONST D3DVERTEXELEMENT9* vertex_elements) : decl(0)
		{
			assert(0 != device);
			core::log::printf("creating vertexdeclaration... ");
			if (FAILED(device->CreateVertexDeclaration(vertex_elements, &decl)))
				throw FatalException("failed to create vertex declaration");
			core::log::printf("done.\n");
		}

		~VertexDeclaration()
		{
			if (decl) decl->Release();
		}

		IDirect3DVertexDeclaration9 *get_vertex_declaration() const {
			return decl;
		}

	private:
		IDirect3DVertexDeclaration9* decl;
	};
}
