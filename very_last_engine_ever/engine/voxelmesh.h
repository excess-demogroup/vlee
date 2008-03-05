#ifndef VOXELMESH_H
#define VOXELMESH_H

#include "voxelgrid.h"
#include "../math/matrix4x4.h"
#include "../renderer/device.h"
#include "../renderer/vertexdeclaration.h"
#include "../renderer/vertexbuffer.h"
#include "../renderer/indexbuffer.h"
#include "../engine/effect.h"

namespace engine
{
	class VoxelMesh
	{
	public:
		VoxelMesh(renderer::Device &device, engine::Effect &effect, const VoxelGrid &voxelGrid, size_t maxSize) :
		  effect(effect),
		  voxelGrid(voxelGrid),
		  maxSize(maxSize),
		  currSize(float(maxSize)),
		  vbSelector(0)
		{
			setupVoxel(device);
			grid = new BYTE[maxSize * maxSize * maxSize];
		}
		
		void setSize(float size)
		{
			if (size > float(maxSize)) currSize = float(maxSize);
			else currSize = size;
		}

		float getSize() const { return currSize; }
		
		void update(const math::Matrix4x4 &mrot);
		void draw(renderer::Device &device) const;
		
	private:
		unsigned char at(int x, int y, int z) const
		{
			return grid[getIndex(x, y, z)];
		}
		
		size_t getIndex(int x, int y, int z) const
		{
			assert(x >= 0);
			assert(y >= 0);
			assert(z >= 0);
			
			assert(x < currSize);
			assert(y < currSize);
			assert(z < currSize);
			
			return
				x + 
				y * maxSize +
				z * maxSize * maxSize;
		}
		
		size_t updateDynamicVertexBuffer(renderer::VertexBuffer &vb);
		
		void fillGrid(math::Matrix4x4 mrot);
		void setupVoxel(renderer::Device &device);
		float getVoxelSize(float dist) const;
		
		BYTE *grid;
		const VoxelGrid &voxelGrid;
		size_t maxSize;
		float currSize;
		
		renderer::VertexDeclaration vertex_decl;
		renderer::VertexBuffer dynamic_vb;
		size_t cubes;
		renderer::VertexBuffer static_vb;
		renderer::IndexBuffer ib;
		engine::Effect &effect;
		int vbSelector;
	};
}

#endif // VOXELMESH_H
