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
		#define SNK 30			  //num of knots per spline loop
		#define SLOD 20			  //level of detail per knot
		#define	SLOOP 350		  //num of spline loops
		#define SCLUSTER (SLOOP/7)//num of SLOOPs clusters together
		

		#define SLERPFACTOR	50	//num of lerps between each lod

		#if defined(SYNC)
		//because of crappy code we need to shorten the anim lenght while using the sync_editor to see stuff :(
			#define SSYNCMAX 0xaa	// animationlength
		#else
			#define SSYNCMAX 0xaa	// animationlength
		#endif

		struct CCBSplineLoop {
			Vector3* knots[SNK];
			Vector3* vectors[SNK*SLOD];
		};

		Grow(VertexStreamer& vs, Vector3& start) : vs(vs), start(start) {
			generateSplineLoops();
		}

		void draw(engine::Effect &effect, float time, int part);
	private:
		void drawFrame(engine::Effect &effect, float time, int part);

		void generateSplineLoops();
		void generateKnots(Vector3& root, Vector3& heading, CCBSplineLoop& sl);
		void generateSplines(CCBSplineLoop& sl);

		VertexStreamer& vs;
		CCBSplineLoop* loops[SLOOP];
		Vector3& start;

		float st0;
		float st2;
		float st3;
		float st4;
		float st5;
		float st6;
	};
}
