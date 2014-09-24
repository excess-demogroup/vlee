#include "stdafx.h"
#include "cubestreamer.h"
#include "effect.h"

using namespace engine;

MeshInstancer::MeshInstancer(renderer::Device &device, engine::Effect *effect, int maxInstanceCount) :
  effect(effect),
  maxInstanceCount(maxInstanceCount)
{
	prepareMeshVertexBuffer(device);
	instance_vb = device.createVertexBuffer(maxInstanceCount * sizeof(float) * 16, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT);
	instanceTransforms = new math::Matrix4x4[maxInstanceCount];
}

MeshInstancer::~MeshInstancer()
{
	delete[] instanceTransforms;
}

void MeshInstancer::draw(renderer::Device &device, int instanceCount) const
{
	// setup render state
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	// setup vs input
	device->SetVertexDeclaration(vertex_decl);
	device->SetStreamSource(0, mesh_vb, 0, 4 * 2);
	device->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | std::min(instanceCount, maxInstanceCount));
	device->SetStreamSource(1, instance_vb, 0, 4 * 3);
	device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1UL);
	device->SetIndices(mesh_ib);
	
	/* draw */
	UINT passes;
	effect->p->Begin(&passes, 0);
	for (UINT pass = 0; pass < passes; ++pass)
	{
		effect->p->BeginPass( pass );
		device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 6 * 4, 0, 6 * 2);
		effect->p->EndPass();
	}
	effect->p->End();

	/* back to normal */
	device->SetStreamSourceFreq(0, 1);
	device->SetStreamSourceFreq(1, 1);
}

void MeshInstancer::updateInstanceVertexBuffer()
{
	float *dst = (float *)instance_vb.lock(0, sizeof(float) * 16 * maxInstanceCount, 0);
	if (dst) {
		memcpy(dst, instanceTransforms, sizeof(float) * 16 * maxInstanceCount);
		instance_vb.unlock();
	}
}

void MeshInstancer::prepareMeshVertexBuffer(renderer::Device &device)
{
	const D3DVERTEXELEMENT9 vertex_elements[] =
	{
		/* static data */
		{ 0, 0, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, // pos
		{ 0, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 }, // normal + front index
		/* instance data */
		{ 1, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // pos2
		{ 1, 4, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 }, // instance array
		{ 1, 8, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 }, // instance array
		{ 1,12, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4 }, // instance array
		D3DDECL_END()
	};
	vertex_decl = device.createVertexDeclaration(vertex_elements);

	const int faces = 6;

	mesh_vb = device.createVertexBuffer(faces * 4 * (4 * 2), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED);
	{
		BYTE *dst = (BYTE*)mesh_vb.lock(0, faces * 4 * (4 * 2), 0);

		/* front face (positive z) */
		*dst++ = 0;   *dst++ = 0;   *dst++ = 255; *dst++ = 255;
		*dst++ = 0;   *dst++ = 0;   *dst++ = 0;   *dst++ = 0; // <0,0>, 0, 0

		*dst++ = 255; *dst++ = 0;   *dst++ = 255; *dst++ = 255;
		*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 0; // <0,0,1>, 0

		*dst++ = 0;   *dst++ = 255; *dst++ = 255; *dst++ = 255;
		*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 0; // <0,0,1>, 0

		*dst++ = 255; *dst++ = 255; *dst++ = 255; *dst++ = 255;
		*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 0; // <0,0,1>, 0

		/* back face (negative z)*/
		*dst++ = 255; *dst++ = 0;   *dst++ = 0; *dst++ = 255;
		*dst++ = 0;   *dst++ = 0;   *dst++ = 0; *dst++ = 1; // <0,0,-1>, 1

		*dst++ = 0;   *dst++ = 0;   *dst++ = 0; *dst++ = 255;
		*dst++ = 255; *dst++ = 0;   *dst++ = 0; *dst++ = 1; // <0,0,-1>, 1

		*dst++ = 255; *dst++ = 255; *dst++ = 0; *dst++ = 255;
		*dst++ = 0;   *dst++ = 255; *dst++ = 0; *dst++ = 1; // <0,0,-1>, 1

		*dst++ = 0;   *dst++ = 255; *dst++ = 0; *dst++ = 255;
		*dst++ = 255; *dst++ = 255; *dst++ = 0; *dst++ = 1; // <0,0,-1>, 1

		/* top face (positive y)*/
		*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 255;
		*dst++ = 0;   *dst++ = 0;   *dst++ = 0; *dst++ = 2; // <0,1,0>, 2

		*dst++ = 0;   *dst++ = 255; *dst++ = 255; *dst++ = 255;
		*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 2; // <0,1,0>, 2

		*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 255;
		*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 2; // <0,1,0>, 2

		*dst++ = 255; *dst++ = 255; *dst++ = 255; *dst++ = 255;
		*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 2; // <0,1,0>, 2

		/* bottom face (negative y) */
		*dst++ = 0;   *dst++ = 0;   *dst++ = 255; *dst++ = 255;
		*dst++ = 0;   *dst++ = 0;   *dst++ = 0;   *dst++ = 3; // <0,-1,0>, 3

		*dst++ = 0;   *dst++ = 0;   *dst++ = 0;   *dst++ = 255;
		*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 3; // <0,-1,0>, 3

		*dst++ = 255; *dst++ = 0;   *dst++ = 255; *dst++ = 255;
		*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 3; // <0,-1,0>, 3

		*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 255;
		*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 3; // <0,-1,0>, 3

		/* left face (positive x)*/
		*dst++ = 255; *dst++ = 0;   *dst++ = 255; *dst++ = 255;
		*dst++ = 0;   *dst++ = 0;   *dst++ = 127; *dst++ = 4; // <1,0,0>, 4

		*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 255;
		*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 4; // <1,0,0>, 4

		*dst++ = 255; *dst++ = 255; *dst++ = 255; *dst++ = 255;
		*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 4; // <1,0,0>, 4

		*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 255;
		*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 4; // <1,0,0>, 4

		/* right face (negative x)*/
		*dst++ = 0;   *dst++ = 0;   *dst++ = 0;   *dst++ = 255;
		*dst++ = 0;   *dst++ = 0;   *dst++ = 0;   *dst++ = 5; // <-1,0,0>, 5

		*dst++ = 0;   *dst++ = 0;   *dst++ = 255; *dst++ = 255;
		*dst++ = 255; *dst++ = 0;   *dst++ = 0;   *dst++ = 5; // <-1,0,0>, 5

		*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 255;
		*dst++ = 0;   *dst++ = 255; *dst++ = 0;   *dst++ = 5; // <-1,0,0>, 5

		*dst++ = 0;   *dst++ = 255; *dst++ = 255; *dst++ = 255;
		*dst++ = 255; *dst++ = 255; *dst++ = 0;   *dst++ = 5; // <-1,0,0>, 5

		mesh_vb.unlock();
	}

	// setup index buffer
	mesh_ib = device.createIndexBuffer(2 * 6 * 6, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED);
	unsigned short *dst = (unsigned short*)mesh_ib.lock(0, 2 * 6 * faces, 0);
	if (dst) {
		for (int i = 0; i < faces; ++i) {
			*dst++ = (i * 4) + 0;
			*dst++ = (i * 4) + 1;
			*dst++ = (i * 4) + 2;
			*dst++ = (i * 4) + 3;
			*dst++ = (i * 4) + 2;
			*dst++ = (i * 4) + 1;
		}
		mesh_ib.unlock();
	}
}

