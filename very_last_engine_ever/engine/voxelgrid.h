#ifndef VOXELGRID_H
#define VOXELGRID_H

#include "../math/math.h"

static unsigned int swizzle_3d_lut[256] = 
{
0x0,
0x1,
0x8,
0x9,
0x40,
0x41,
0x48,
0x49,
0x200,
0x201,
0x208,
0x209,
0x240,
0x241,
0x248,
0x249,
0x1000,
0x1001,
0x1008,
0x1009,
0x1040,
0x1041,
0x1048,
0x1049,
0x1200,
0x1201,
0x1208,
0x1209,
0x1240,
0x1241,
0x1248,
0x1249,
0x8000,
0x8001,
0x8008,
0x8009,
0x8040,
0x8041,
0x8048,
0x8049,
0x8200,
0x8201,
0x8208,
0x8209,
0x8240,
0x8241,
0x8248,
0x8249,
0x9000,
0x9001,
0x9008,
0x9009,
0x9040,
0x9041,
0x9048,
0x9049,
0x9200,
0x9201,
0x9208,
0x9209,
0x9240,
0x9241,
0x9248,
0x9249,
0x40000,
0x40001,
0x40008,
0x40009,
0x40040,
0x40041,
0x40048,
0x40049,
0x40200,
0x40201,
0x40208,
0x40209,
0x40240,
0x40241,
0x40248,
0x40249,
0x41000,
0x41001,
0x41008,
0x41009,
0x41040,
0x41041,
0x41048,
0x41049,
0x41200,
0x41201,
0x41208,
0x41209,
0x41240,
0x41241,
0x41248,
0x41249,
0x48000,
0x48001,
0x48008,
0x48009,
0x48040,
0x48041,
0x48048,
0x48049,
0x48200,
0x48201,
0x48208,
0x48209,
0x48240,
0x48241,
0x48248,
0x48249,
0x49000,
0x49001,
0x49008,
0x49009,
0x49040,
0x49041,
0x49048,
0x49049,
0x49200,
0x49201,
0x49208,
0x49209,
0x49240,
0x49241,
0x49248,
0x49249,
0x200000,
0x200001,
0x200008,
0x200009,
0x200040,
0x200041,
0x200048,
0x200049,
0x200200,
0x200201,
0x200208,
0x200209,
0x200240,
0x200241,
0x200248,
0x200249,
0x201000,
0x201001,
0x201008,
0x201009,
0x201040,
0x201041,
0x201048,
0x201049,
0x201200,
0x201201,
0x201208,
0x201209,
0x201240,
0x201241,
0x201248,
0x201249,
0x208000,
0x208001,
0x208008,
0x208009,
0x208040,
0x208041,
0x208048,
0x208049,
0x208200,
0x208201,
0x208208,
0x208209,
0x208240,
0x208241,
0x208248,
0x208249,
0x209000,
0x209001,
0x209008,
0x209009,
0x209040,
0x209041,
0x209048,
0x209049,
0x209200,
0x209201,
0x209208,
0x209209,
0x209240,
0x209241,
0x209248,
0x209249,
0x240000,
0x240001,
0x240008,
0x240009,
0x240040,
0x240041,
0x240048,
0x240049,
0x240200,
0x240201,
0x240208,
0x240209,
0x240240,
0x240241,
0x240248,
0x240249,
0x241000,
0x241001,
0x241008,
0x241009,
0x241040,
0x241041,
0x241048,
0x241049,
0x241200,
0x241201,
0x241208,
0x241209,
0x241240,
0x241241,
0x241248,
0x241249,
0x248000,
0x248001,
0x248008,
0x248009,
0x248040,
0x248041,
0x248048,
0x248049,
0x248200,
0x248201,
0x248208,
0x248209,
0x248240,
0x248241,
0x248248,
0x248249,
0x249000,
0x249001,
0x249008,
0x249009,
0x249040,
0x249041,
0x249048,
0x249049,
0x249200,
0x249201,
0x249208,
0x249209,
0x249240,
0x249241,
0x249248,
0x249249
};

#include <limits.h>

namespace engine
{
	class VoxelGrid
	{
	public:
		VoxelGrid(size_t grid_size) :
		  distances(NULL),
		  max_distances(NULL),
		  min_distances(NULL),
		  grid_size(grid_size)
		{
			distances = new signed char[grid_size * grid_size * grid_size];
			max_distances = new signed char[grid_size * grid_size * grid_size];
			min_distances = new signed char[grid_size * grid_size * grid_size];
			memset(distances, 0, sizeof(signed char) *grid_size * grid_size * grid_size);
			memset(max_distances, CHAR_MIN, sizeof(signed char) * grid_size * grid_size * grid_size);
			memset(min_distances, CHAR_MAX, sizeof(signed char) * grid_size * grid_size * grid_size);
		}

		inline int getIndex(int x, int y, int z) const
		{
			return swizzle_3d_lut[x] | (swizzle_3d_lut[y] << 1) | (swizzle_3d_lut[z] << 2);
//			return (z * grid_size + y) * grid_size + x;
		}

		inline signed char pointSample(int x, int y, int z) const
		{
			assert(x >= 0);
			assert(x < int(grid_size));
			assert(y >= 0);
			assert(y < int(grid_size));
			assert(z >= 0);
			assert(z < int(grid_size));
			return distances[getIndex(x, y, z)];
		}

#if 1
		float trilinearSample(float x, float y, float z) const
		{
			int ix = int(floor(x));
			int iy = int(floor(y));
			int iz = int(floor(z));

			signed char f0 = pointSample(ix,     iy,     iz);
			signed char f1 = pointSample(ix + 1, iy,     iz);
			signed char f2 = pointSample(ix,     iy + 1, iz);
			signed char f3 = pointSample(ix + 1, iy + 1, iz);

			signed char b0 = pointSample(ix,     iy,     iz + 1);
			signed char b1 = pointSample(ix + 1, iy,     iz + 1);
			signed char b2 = pointSample(ix,     iy + 1, iz + 1);
			signed char b3 = pointSample(ix + 1, iy + 1, iz + 1);

			float xt = (x - ix);
			float yt = (y - iy);
			float zt = (z - iz);

			float a = (1 - xt) * (1 - yt);
			float b = (    xt) * (1 - yt);
			float c = (1 - xt) * (    yt);
			float d = 1 - a - b - c;
			float e = (1 - zt);
			float f = (    zt);

			float v = 0.0f;
			v += f0 * a * e;
			v += f1 * b * e;
			v += f2 * c * e;
			v += f3 * d * e;

			v += b0 * a * f;
			v += b1 * b * f;
			v += b2 * c * f;
			v += b3 * d * f;
			
			return v;
		}
#else
		float trilinearSample(float x, float y, float z) const
		{
			int ix = int(floor(x));
			int iy = int(floor(y));
			int iz = int(floor(z));

			signed char f0 = pointSample(ix,     iy,     iz);
			signed char f1 = pointSample(ix + 1, iy,     iz);
			signed char f2 = pointSample(ix,     iy + 1, iz);
			signed char f3 = pointSample(ix + 1, iy + 1, iz);

			signed char b0 = pointSample(ix,     iy,     iz + 1);
			signed char b1 = pointSample(ix + 1, iy,     iz + 1);
			signed char b2 = pointSample(ix,     iy + 1, iz + 1);
			signed char b3 = pointSample(ix + 1, iy + 1, iz + 1);

			float xt = (x - ix);
			float yt = (y - iy);
			float zt = (z - iz);

			float a = (1 - xt);
			float b = (    xt);


			float y1, y2;

			/* first layer */
			y1 = math::lerp(float(f0), float(f1), xt);
			y2 = math::lerp(float(f2), float(f3), xt);
			float z1 = math::lerp(y1, y2, yt);

			/* second layer */
			y1 = math::lerp(float(b0), float(b1), xt);
			y2 = math::lerp(float(b2), float(b3), xt);
			float z2 = math::lerp(y1, y2, yt);

			return math::lerp(z1, z2, zt);
		}
#endif

		float trilinearSample(int x, int y, int z) const
		{
			int ix = x >> 24;
			int iy = y >> 24;
			int iz = z >> 24;
			
			int f0 = pointSample(ix,     iy,     iz);
			int f1 = pointSample(ix + 1, iy,     iz);
			int f2 = pointSample(ix,     iy + 1, iz);
			int f3 = pointSample(ix + 1, iy + 1, iz);
			
			int b0 = pointSample(ix,     iy,     iz + 1);
			int b1 = pointSample(ix + 1, iy,     iz + 1);
			int b2 = pointSample(ix,     iy + 1, iz + 1);
			int b3 = pointSample(ix + 1, iy + 1, iz + 1);
			
			int ixt = (x & ((1 << 24) - 1)) >> 8;
			int iyt = (y & ((1 << 24) - 1)) >> 8;
			int izt = (z & ((1 << 24) - 1)) >> 8;
			
			float xt = float(ixt) / (1 << 16);
			float yt = float(iyt) / (1 << 16);
			float zt = float(izt) / (1 << 16);
			
			float y1, y2;
			
			/* first layer */
			y1 = math::lerp(float(f0), float(f1), xt);
			y2 = math::lerp(float(f2), float(f3), xt);
			float z1 = math::lerp(float(y1), float(y2), yt);
			
			/* second layer */
			y1 = math::lerp(float(b0), float(b1), xt);
			y2 = math::lerp(float(b2), float(b3), xt);
			float z2 = math::lerp(float(y1), float(y2), yt);
			
			return math::lerp(z1, z2, zt);
		}
		
		void setDistance(int x, int y, int z, float dist)
		{
			assert(x >= 0);
			assert(x < int(grid_size));
			assert(y >= 0);
			assert(y < int(grid_size));
			assert(z >= 0);
			assert(z < int(grid_size));
			
			signed char cdist = (signed char)dist;
			distances[getIndex(x, y, z)] = cdist;

			updateMinMax(x,   y,   z, cdist);
			updateMinMax(x-1, y,   z, cdist);
			updateMinMax(x,   y-1, z, cdist);
			updateMinMax(x-1, y-1, z, cdist);
			updateMinMax(x,   y,   z-1, cdist);
			updateMinMax(x-1, y,   z-1, cdist);
			updateMinMax(x,   y-1, z-1, cdist);
			updateMinMax(x-1, y-1, z-1, cdist);
		}

		void updateMinMax(int x, int y, int z, signed char dist)
		{
			if (x < 0) return;
			if (x >= int(grid_size)) return;
			if (y < 0) return;
			if (y >= int(grid_size)) return;
			if (z < 0) return;
			if (z >= int(grid_size)) return;
			
			int index = getIndex(x, y, z);
			if (min_distances[index] > dist) min_distances[index] = dist;
			if (max_distances[index] < dist) max_distances[index] = dist;
		}

		size_t getSize() const { return grid_size; }

	private:
		size_t grid_size;
		signed char *distances;
	public:
		signed char *max_distances;
		signed char *min_distances;

		float max_dist;
	};
	
	VoxelGrid loadVoxelGrid(std::string fileName);
}

#endif /* VOXELGRID_H */
