#include "stdafx.h"
#include "particlestreamer.h"
#include "../core/err.h"

using engine::ParticleStreamer;

ParticleStreamer::ParticleStreamer(renderer::Device &device) :
	device(device),
	locked_pointer(NULL)
{
	const int static_vb_size = PARTICLE_STREAMER_VERTEX_COUNT * 2 * sizeof(float);

	static_vb  = renderer::VertexBuffer(device, static_vb_size,     D3DUSAGE_WRITEONLY,                    D3DPOOL_DEFAULT);
	assert(NULL != static_vb);
	assert(PARTICLE_STREAMER_VERTEX_COUNT == (PARTICLE_STREAMER_PARTICLE_COUNT * 4));
	{
		float *dst = (float *)static_vb.lock(0, static_vb_size, 0);
		for (int i = 0; i < PARTICLE_STREAMER_PARTICLE_COUNT; ++i)
		{
			*dst++ = -1.f;
			*dst++ = -1.f;

			*dst++ = -1.f;
			*dst++ =  1.f;

			*dst++ =  1.f;
			*dst++ =  1.f;

			*dst++ =  1.f;
			*dst++ = -1.f;
		}
		static_vb.unlock();
	}

	dynamic_vb = renderer::VertexBuffer(device, PARTICLE_STREAMER_VERTEX_COUNT * 4 * sizeof(float), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DPOOL_DEFAULT);

	const int index_buffer_size = PARTICLE_STREAMER_PARTICLE_COUNT * sizeof(unsigned short) * 6;
	indices = renderer::IndexBuffer(device, index_buffer_size, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16);
	{
		unsigned short *dst = (unsigned short *)indices.lock(0, index_buffer_size, 0);
		for (int i = 0; i < PARTICLE_STREAMER_PARTICLE_COUNT; ++i)
		{
			*dst++ = i * 4 + 0;
			*dst++ = i * 4 + 1;
			*dst++ = i * 4 + 2;

			*dst++ = i * 4 + 2;
			*dst++ = i * 4 + 3;
			*dst++ = i * 4 + 0;
		}
		indices.unlock();
	}

	const D3DVERTEXELEMENT9 vertex_elements[] =
	{
		{ 0, 0 * sizeof(float), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 3 * sizeof(float), D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
		{ 1, 0 * sizeof(float), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
		D3DDECL_END()
	};

	vertex_decl = device.createVertexDeclaration(vertex_elements);
}

void ParticleStreamer::draw()
{
	int max_vertex      = PARTICLE_STREAMER_VERTEX_COUNT - (locked_particles * 4);
	int primitive_count = 2 * (PARTICLE_STREAMER_PARTICLE_COUNT - locked_particles);

	if (primitive_count == 0) return;

	device->SetIndices(indices);
	core::d3dErr(device->SetFVF(0));
	core::d3dErr(device->SetVertexDeclaration(vertex_decl));

	device->SetStreamSource(0, dynamic_vb, 0, 4 * sizeof(float));
	device->SetStreamSource(1, static_vb,  0, 2 * sizeof(float));

	core::d3dErr(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, max_vertex, 0, primitive_count));
}
