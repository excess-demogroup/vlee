#ifndef MESHINSTANCER_H
#define MESHINSTANCER_H

#include "voxelgrid.h"
#include "../math/matrix4x4.h"
#include "../renderer/device.h"
#include "../renderer/vertexdeclaration.h"
#include "../renderer/vertexbuffer.h"
#include "../renderer/indexbuffer.h"

namespace engine
{
	class Effect;

	class MeshInstancer
	{
	public:
		MeshInstancer(renderer::Device &device, engine::Effect *effect, int maxInstanceCount);
		~MeshInstancer();

		void draw(renderer::Device &device, int instanceCount) const;
		void setInstanceTransform(int instance, const math::Matrix4x4 &transform)
		{
			if (instance < maxInstanceCount)
				instanceTransforms[instance] = transform;
		}
		void updateInstanceVertexBuffer();
		
	private:
		void prepareMeshVertexBuffer(renderer::Device &device);

		math::Matrix4x4 *instanceTransforms;
		renderer::VertexDeclaration vertex_decl;
		renderer::VertexBuffer instance_vb;
		renderer::VertexBuffer mesh_vb;
		renderer::IndexBuffer mesh_ib;
		engine::Effect *effect;
		int maxInstanceCount;
	};
}

#endif // MESHINSTANCER_H
