#pragma once

#include "effect.h"
#include "vertexstreamer.h"
#include "../math/vector3.h"
#include "../renderer/device.h"
using math::Vector3;

namespace engine
{
	class Grow {
	public:
		#define SNK 15			//num of knots per spline loop
		#define SLOD 4			//level of detail per knot
		#define	SLOOP 50		//num of spline loops
		#define SLERPFACTOR	50	//num of lerps between each lod

		#define SSYNCMAX 4000	// num in tracker the gives "complete animation"

		struct CCBSplineLoop {
			Vector3* knots[SNK];
			Vector3* vectors[SNK*SLOD];
		};

		Grow(VertexStreamer& vs, Vector3& start, Vector3& stop) : vs(vs), start(start), stop(stop) {
			generateSplineLoops();
		}

		void draw(engine::Effect &effect, float time);
	private:
		void drawFrame(engine::Effect &effect, float time);

		void generateSplineLoops();
		void generateKnots(int mod, CCBSplineLoop& sl);
		void generateSplines(CCBSplineLoop& sl);

		VertexStreamer& vs;
		CCBSplineLoop* loops[SLOOP];
		Vector3& start;
		Vector3& stop;

	};
}
