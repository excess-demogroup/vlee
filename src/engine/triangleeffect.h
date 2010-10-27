#pragma once

#include "effect.h"
#include "vertexstreamer.h"

namespace engine
{
	class TriangleEffect {
	public:
		TriangleEffect() {};

		void draw(engine::Effect &effect, engine::VertexStreamer &streamer, double beat, unsigned numOfTris, float size, float dist, float shaper, float opening);
	};
}
