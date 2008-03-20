#include "stdafx.h"
#include "Grow.h"
#include "../core/err.h"
#include "../math/notrand.h"
using engine::Grow;
using namespace math;
#include <algorithm>
using namespace std;


void Grow::draw(engine::Effect &effect, float time) {
	if ((int)time == 0) return;
	UINT passes;
	effect->Begin(&passes, 0);
	for (UINT pass = 0; pass < passes; ++pass)
	{
		effect->BeginPass( pass );
		drawFrame(effect,time);
		effect->EndPass();
	}
	effect->End();
}

void Grow::drawFrame(engine::Effect &effect, float time) {
	float completeness = min(time,(float)SSYNCMAX) / (float)SSYNCMAX;
	int frame = (int)((SNK*SLOD*SLERPFACTOR)*completeness);
	int it = (int)((SNK*SLOD)*completeness);
	int step = (int)(frame/it);

	for (int i = 0; i < SLOOP; ++i) {
		
		vs.begin(D3DPT_LINELIST);
		for (int c = 0; c < it-1; ++c) {
			vs.vertex(*loops[i]->vectors[c%(SNK*SLOD)]);
			vs.vertex(*loops[i]->vectors[(c+1)%(SNK*SLOD)]);
		}
		Vector3 v1 = *loops[i]->vectors[it-1%(SNK*SLOD)];
		Vector3 v2 = *loops[i]->vectors[it%(SNK*SLOD)];
		Vector3 newend;
		float factor = (1/SLERPFACTOR)*(step%SLERPFACTOR+1);
		D3DXVec3Lerp(&newend, &v1, &v2, factor);

		vs.vertex(v1);
		vs.vertex(newend);
		vs.end();
	}
}



void Grow::generateSplineLoops() {
	for (int i = 0; i < SLOOP; ++i){
		CCBSplineLoop* sl = new CCBSplineLoop();
		generateKnots(i, *sl);
		generateSplines(*sl);
		loops[i] = sl;
	}
}

void Grow::generateKnots(int mod, CCBSplineLoop& sl) {
	float lerpmod = (1.f/SLOOP)*(mod+1.f);
	Vector3 v;
	D3DXVec3Lerp(&v, &start, &stop, lerpmod);
	float y = v.y;
	for (int i = 0; i < SNK; ++i) {
		y += i * ((rand() * (1.f / RAND_MAX))/SNK);
		sl.knots[i] = new Vector3(v.x, y, v.z+(rand() * (1.f / RAND_MAX))*0.5f);

	}
}

void Grow::generateSplines(CCBSplineLoop& sl) {
	for (int c = 0; c < SNK; ++c){
		for (int i = 0; i < SLOD; ++i) {
			float t = (float)i / (float)SLOD;
			float it= (float)1.0-t;

			float b0 = it*it*it/6.f;
			float b1 = (3.f*t*t*t-6.f*t*t+4.f)/6.f;
			float b2 = (-3.f*t*t*t+3.f*t*t+3.f*t+1)/6.f;
			float b3 = t*t*t/6.f;
	
			float x = b0*sl.knots[(c+0)%SNK]->x+ 
					  b1*sl.knots[(c+1)%SNK]->x+
					  b2*sl.knots[(c+2)%SNK]->x+ 
					  b3*sl.knots[(c+3)%SNK]->x;
			float y = b0*sl.knots[(c+0)%SNK]->y+
				      b1*sl.knots[(c+1)%SNK]->y+ 
					  b2*sl.knots[(c+2)%SNK]->y+
					  b3*sl.knots[(c+3)%SNK]->y;
			float z = b0*sl.knots[(c+0)%SNK]->z+
				      b1*sl.knots[(c+1)%SNK]->z+
					  b2*sl.knots[(c+2)%SNK]->z+ 
					  b3*sl.knots[(c+3)%SNK]->z;

			sl.vectors[c*SLOD+i] = new Vector3(x,y,z);
		}
	}
}



