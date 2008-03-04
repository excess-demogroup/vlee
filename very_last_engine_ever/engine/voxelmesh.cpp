#include "stdafx.h"
#include "voxelmesh.h"
#include "../math/vector3.h"

using namespace engine;
using math::Vector3;

float VoxelMesh::getVoxelSize(float dist) const
{
	dist /= 128.0f;
	dist *= sqrtf(float(voxelGrid.getSize() * voxelGrid.getSize() * voxelGrid.getSize())) / 2;
	return (0.5f / voxelGrid.getSize()) - dist * 0.5f;
}

void VoxelMesh::update(const math::Matrix4x4 &mrot)
{
	fillGrid(mrot);
	renderer::VertexBuffer &vb = dynamic_vb;
	cubes = updateDynamicVertexBuffer(vb);
}

void VoxelMesh::draw(renderer::Device &device) const
{
	// setup render state
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	// setup vs input
	device->SetVertexDeclaration(vertex_decl);
	device->SetStreamSource(0, static_vb, 0, 4 * 2);
	device->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | cubes);
	device->SetStreamSource(1, dynamic_vb, 0, 4 * 3);
	device->SetStreamSourceFreq(1, D3DSTREAMSOURCE_INSTANCEDATA | 1UL);
	device->SetIndices(ib);
	
	/* draw */
	UINT passes;
	effect->Begin(&passes, 0);
	for (UINT pass = 0; pass < passes; ++pass)
	{
		effect->BeginPass( pass );
		device->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 6 * 4, 0, 6 * 2);
		effect->EndPass();
	}
	effect->End();

	/* back to normal */
	device->SetStreamSourceFreq(0, 1);
	device->SetStreamSourceFreq(1, 1);
}

void VoxelMesh::fillGrid(math::Matrix4x4 mrot)
{
	int igrid_min_size = int(floor(currSize) / 2);
	int igrid_max_size = int(ceil(currSize) / 2);

#if 0
	igrid_min_size /= 4;
	igrid_max_size /= 4;
#endif

	float translate = float(voxelGrid.getSize()) / 2;
	float scale = float(voxelGrid.getSize()) / 2;

	math::Matrix4x4 mtranslate, mscale;
	mtranslate.make_translation(math::Vector3(translate, translate, translate));
	mscale.make_scaling(math::Vector3(scale, scale, scale));
	mrot *= mscale * mtranslate;

	float grid_size_rcp = 1.0f / (currSize / 2);

	Vector3 dx = Vector3(mrot._11, mrot._12, mrot._13) * grid_size_rcp;
	Vector3 dy = Vector3(mrot._21, mrot._22, mrot._23) * grid_size_rcp;
	Vector3 dz = Vector3(mrot._31, mrot._32, mrot._33) * grid_size_rcp;

	Vector3 pz(
		float(-igrid_min_size) * grid_size_rcp,
		float(-igrid_min_size) * grid_size_rcp,
		float(-igrid_min_size) * grid_size_rcp
		);
	pz = math::mul(mrot, pz);

	int dx_x = int(dx.x * (1 << 24));
	int dx_y = int(dx.y * (1 << 24));
	int dx_z = int(dx.z * (1 << 24));

	// find the lowest and highest min/max values that can contribute to a result
	int min_threshold = SCHAR_MIN;
	int max_threshold  = SCHAR_MAX;
	for (int i = SCHAR_MIN; i < SCHAR_MAX; ++i)
	{
		float size = getVoxelSize(float(i));
		if (size > 0.0f) min_threshold = std::max(min_threshold, i);
		if (size < 1.0f) max_threshold = std::min(max_threshold, i);
	}

	for (int z = -igrid_min_size; z < igrid_max_size; ++z)
	{
		Vector3 py = pz;
		for (int y = -igrid_min_size; y < igrid_max_size; ++y)
		{
			int px_x = int(py.x * (1 << 24));
			int px_y = int(py.y * (1 << 24));
			int px_z = int(py.z * (1 << 24));

			for (int x = -igrid_min_size; x < igrid_max_size; ++x)
			{
				int px = px_x;
				int py = px_y;
				int pz = px_z;

				px_x += dx_x;
				px_y += dx_y;
				px_z += dx_z;

				int ix = px >> 24;
				int iy = py >> 24;
				int iz = pz >> 24;

				int xx = x + igrid_min_size;
				int yy = y + igrid_min_size;
				int zz = z + igrid_min_size;
				size_t dstIndex = getIndex(xx, yy, zz);
				
				if (
					ix <= 0 || ix >= int(voxelGrid.getSize()) - 1 ||
					iy <= 0 || iy >= int(voxelGrid.getSize()) - 1 ||
					iz <= 0 || iz >= int(voxelGrid.getSize()) - 1)
				{
					grid[dstIndex] = 0;
					continue;
				}

				int srcIndex = voxelGrid.getIndex(ix, iy, iz);

				if (voxelGrid.min_distances[srcIndex] > min_threshold)
				{
					grid[dstIndex] = 0;
					continue;
				}
				if (voxelGrid.max_distances[srcIndex] < max_threshold)
				{
					grid[dstIndex] = 255;
					continue;
				}

				float size = getVoxelSize(voxelGrid.trilinearSample(px, py, pz));
				//				size = 0.15f;
				grid[dstIndex] = BYTE(math::clamp(size, 0.0f, 1.0f) * 255);
			}
			py += dy;
		}
		pz += dz;
	}
}

size_t VoxelMesh::updateDynamicVertexBuffer(renderer::VertexBuffer &vb)
{
	int cubes = 0;
	int culled = 0;

	int igrid_min_size = int(floor(currSize) / 2);
	int igrid_max_size = int(ceil(currSize) / 2);
	int igrid_size = igrid_max_size + igrid_min_size;


	BYTE *dst = (BYTE*)vb.lock(0, igrid_size * igrid_size * igrid_size * (4 * 3), 0);
	for (int z = 0; z < igrid_size; ++z)
	{
		for (int y = 0; y < igrid_size; ++y)
		{
			for (int x = 0; x < igrid_size; ++x)
			{
				int srcIndex = getIndex(x, y, z);
				BYTE size = grid[srcIndex];

				if (size == 0) continue;
				if (
					(x > 0 && x < igrid_size - 1) &&
					(y > 0 && y < igrid_size - 1) &&
					(z > 0 && z < igrid_size - 1)
					)
				{

					if (
						at(x-1, y, z) == 255 &&
						at(x+1, y, z) == 255 &&
						at(x, y-1, z) == 255 &&
						at(x, y+1, z) == 255 &&
						at(x, y, z-1) == 255 &&
						at(x, y, z+1) == 255
						)
					{
						culled++;
						continue;
					}
				}

				*dst++ = x; *dst++ = y; *dst++ = z;
				*dst++ = size;

				/* neighbour info: 6 centers, 12 edges, 8 corners */
				/* x = center, y = even edge, z = odd edge, w = */
				/* tc0 - x y z w */
				/* tc1 - x y z w */
				/* tc2 - x y z w */
				/* tc3 - x y z w */
				/* tc4 - x y z w */
				/* tc5 - x y z w */
				/* tc6 - x y z w */

				/* fill in centre faces */
				*dst++ = z < igrid_size - 1 ? at(x, y, z+1) : 0; // +z
				*dst++ = z > 0 ?              at(x, y, z-1) : 0; // -z

				*dst++ = y < igrid_size - 1 ? at(x, y+1, z) : 0; // +y
				*dst++ = y > 0 ?              at(x, y-1, z) : 0; // -y

				*dst++ = x < igrid_size - 1 ? at(x+1, y, z) : 0; // +x
				*dst++ = x > 0 ?              at(x-1, y, z) : 0; // -x

#if 0
				/* fill in edge faces */
				// front layer (z+1)
				*dst++ = (y < igrid_size - 1 && z < igrid_size - 1) ? grid[z+1][y+1][x] : 0; // +y +z
				*dst++ = (y > 0              && z < igrid_size - 1) ? grid[z+1][y-1][x] : 0; // -y +z
				*dst++ = (x < igrid_size - 1 && z < igrid_size - 1) ? grid[z+1][y][x+1] : 0; // +x +z
				*dst++ = (x > 0              && z < igrid_size - 1) ? grid[z+1][y][x-1] : 0; // -x +z

				// middle layer (z)
				*dst++ = (x > 0              && y > 0             ) ? grid[z][y-1][x-1] : 0; // -x -y
				*dst++ = (x < igrid_size - 1 && y > 0             ) ? grid[z][y-1][x+1] : 0; // +x -y
				*dst++ = (x > 0              && y < igrid_size - 1) ? grid[z][y+1][x-1] : 0; // -x +y
				*dst++ = (x < igrid_size - 1 && y < igrid_size - 1) ? grid[z][y+1][x+1] : 0; // +x +y

				// bottom layer (z-1)
				*dst++ = (y < igrid_size - 1 && z > 0             ) ? grid[z-1][y+1][x] : 0; // +y -z
				*dst++ = (y > 0              && z > 0             ) ? grid[z-1][y-1][x] : 0; // -y -z
				*dst++ = (x < igrid_size - 1 && z > 0             ) ? grid[z-1][y][x+1] : 0; // +x -z
				*dst++ = (x > 0              && z > 0             ) ? grid[z-1][y][x-1] : 0; // -x -z
#endif

				/* fill in corners (?) */

				*dst++ = 128;
				*dst++ = 128;

				cubes++;
			}
		}
	}
	vb.unlock();
	return cubes;
}

void engine::VoxelMesh::setupVoxel(renderer::Device &device)
{
	const D3DVERTEXELEMENT9 vertex_elements[] =
	{
		/* static data */
		{ 0, 0, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, // pos
		{ 0, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 }, // normal + front index
		/* instance data */
		{ 1, 0, D3DDECLTYPE_UBYTE4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, // pos2
		{ 1, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 }, // instance array
		{ 1, 8, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 }, // instance array
		D3DDECL_END()
	};
	vertex_decl = device.createVertexDeclaration(vertex_elements);

	const int faces = 6;

	static_vb  = device.createVertexBuffer(faces * 4 * (4 * 2), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED);
	{
		BYTE *dst = (BYTE*)static_vb.lock(0, faces * 4 * (4 * 2), 0);

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

		static_vb.unlock();
	}

	// setup index buffer
	ib = device.createIndexBuffer(2 * 6 * 6, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED);
	unsigned short *dst = (unsigned short*)ib.lock(0, 2 * 6 * faces, 0);
	if (NULL != dst)
	{
		for (int i = 0; i < faces; ++i)
		{
			*dst++ = (i * 4) + 0;
			*dst++ = (i * 4) + 1;
			*dst++ = (i * 4) + 2;
			*dst++ = (i * 4) + 3;
			*dst++ = (i * 4) + 2;
			*dst++ = (i * 4) + 1;
		}
		ib.unlock();
	}

	dynamic_vb = device.createVertexBuffer(maxSize * maxSize * maxSize * (4 * 3), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT);
}

