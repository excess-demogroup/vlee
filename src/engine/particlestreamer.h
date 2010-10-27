#pragma once

#include "../renderer/device.h"
#include "../renderer/vertexbuffer.h"
#include "../renderer/indexbuffer.h"
#include "../renderer/vertexdeclaration.h"

#include "../math/vector3.h"

#include "drawable.h"

#define PARTICLE_STREAMER_PARTICLE_COUNT 1024
#define PARTICLE_STREAMER_VERTEX_COUNT (PARTICLE_STREAMER_PARTICLE_COUNT * 4)
namespace engine
{
	class ParticleStreamer : public Drawable
	{
	public:
		ParticleStreamer() {}
		ParticleStreamer(renderer::Device &device);

		renderer::VertexBuffer static_vb;
		renderer::VertexBuffer dynamic_vb;
		renderer::IndexBuffer  indices;
		renderer::VertexDeclaration vertex_decl;

		void begin()
		{
			locked_pointer = (float*)dynamic_vb.lock(0, PARTICLE_STREAMER_VERTEX_COUNT * 4 * sizeof(float), D3DLOCK_DISCARD);
			locked_particles = PARTICLE_STREAMER_PARTICLE_COUNT;
		}

		inline void add(const math::Vector3 &pos, const float size)
		{
			assert(NULL != locked_pointer);
			assert(0 != locked_particles);
			
			for (int i = 0; i < 4; ++i)
			{
				*locked_pointer++ = pos.x;
				*locked_pointer++ = pos.y;
				*locked_pointer++ = pos.z;
				*locked_pointer++ = size;
			}
			locked_particles--;
		}

		void end()
		{
			dynamic_vb.unlock();
			locked_pointer = NULL;
		}

		void draw();

	private:
		renderer::Device device;
		int    locked_particles;
		float *locked_pointer;
	};
}
