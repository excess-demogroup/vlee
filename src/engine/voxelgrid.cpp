#include "stdafx.h"
#include "voxelgrid.h"
#include "../core/fatalexception.h"

using namespace engine;
#define VOXEL_DATA_SIZE 32

VoxelGrid engine::loadVoxelGrid(std::string fileName)
{
	FILE *fp = fopen(fileName.c_str(), "rb");
	if (NULL == fp) throw core::FatalException("failed to load voxel");
	
	engine::VoxelGrid voxelgrid(VOXEL_DATA_SIZE);
	fread(&voxelgrid.max_dist, 4, 1, fp);
	for (int z = 0; z < VOXEL_DATA_SIZE; ++z)
	{
		for (int y = 0; y < VOXEL_DATA_SIZE; ++y)
		{
			for (int x = 0; x < VOXEL_DATA_SIZE; ++x)
			{
				signed char bdist;
				fread(&bdist, 1, 1, fp);
				voxelgrid.setDistance(x, y, z, bdist);
			}
		}
	}
	fclose(fp);
	return voxelgrid;
}
