#ifndef VOXELGRID_H
#define VOXELGRID_H

namespace engine
{

	template <int grid_size>
	class VoxelGrid
	{
	public:
		inline signed char pointSample(int x, int y, int z)
		{
			assert(x >= 0);
			assert(x < grid_size);
			assert(y >= 0);
			assert(y < grid_size);
			assert(z >= 0);
			assert(z < grid_size);
			return distances[z][y][x];
		}

		float trilinearSample(float x, float y, float z)
		{
	//		x = floor(x);

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

			return v / 128;
		}

		void setDistance(int x, int y, int z, float dist)
		{
			assert(x >= 0);
			assert(x < grid_size);
			assert(y >= 0);
			assert(y < grid_size);
			assert(z >= 0);
			assert(z < grid_size);
			
			distances[z][y][x] = (signed char)dist;
		}

	private:
		signed char distances[grid_size][grid_size][grid_size];
	};
}

#endif /* VOXELGRID_H */
