#include "stdafx.h"
#include "Grow.h"
#include "../core/err.h"
#include "../math/notrand.h"
using engine::Grow;
using namespace math;
#include <algorithm>
using namespace std;


void Grow::draw(engine::Effect &effect, int time) {
	if (time == 0) return;
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

void Grow::drawFrame(engine::Effect &effect, int time) {
	time = (int)(time/10);
	for (int i = 0; i < SLOOP; ++i) {
		vs.begin(D3DPT_LINELIST);
		for (int c = 0; c < min(time,SLOD*SNK); ++c) {
			vs.vertex(*loops[i]->vectors[c%(SNK*SLOD)]);
			vs.vertex(*loops[i]->vectors[(c+1)%(SNK*SLOD)]);
		}
		vs.end();
	}
}



void Grow::generateSplineLoops() {
	len = math::length(stop-start)/SLOOP;
	for (int i = 0; i < SLOOP; ++i){
		CCBSplineLoop* sl = new CCBSplineLoop();
		generateKnots(i, *sl);
		generateSplines(*sl);
		loops[i] = sl;
	}
}

void Grow::generateKnots(int mod, CCBSplineLoop& sl) {
	Vector3 v = start+(mod*len)*(stop-start);
	float y = v.y;
	for (int i = 0; i < SNK; ++i) {
		y += i * ((rand() * (1.f / RAND_MAX))/SNK);
		sl.knots[i] = new Vector3(v.x, y, v.z+(rand() * (1.f / RAND_MAX))-0.5f);

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
	
			float x = b0*sl.knots[(c-0)%SNK]->x + b1*sl.knots[(c+1)%SNK]->x + b2*sl.knots[(c+2)%SNK]->x + b3*sl.knots[(c+3)%SNK]->x;
			float y = b0*sl.knots[(c-0)%SNK]->y + b1*sl.knots[(c+1)%SNK]->y + b2*sl.knots[(c+2)%SNK]->y + b3*sl.knots[(c+3)%SNK]->y;
			float z = b0*sl.knots[(c-0)%SNK]->z + b1*sl.knots[(c+1)%SNK]->z + b2*sl.knots[(c+2)%SNK]->z + b3*sl.knots[(c+3)%SNK]->z;

			sl.vectors[c*SLOD+i] = new Vector3(x,y,z);
		}
	}
}



