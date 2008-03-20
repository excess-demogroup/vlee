#include "stdafx.h"
#include "ccbsplines.h"
#include "../core/err.h"
#include "../math/notrand.h"
using engine::CCBSplines;
using namespace math;


void CCBSplines::draw(engine::Effect &effect, double time) {
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

void CCBSplines::drawFrame(engine::Effect &effect, double time) {
	time *= 2;
	ps.begin();
	for (int i = 0; i < CSLOOP; ++i) {
		for (int c = time; c < time+CSLOD; ++c)
			ps.add(loops[i].vectors[c%(CSNK*CSLOD)], 0.5f);
	}
	ps.end();
	ps.draw();
}



void CCBSplines::generateSplineLoops() {
	for (int i = 0; i < CSLOOP; ++i){
		CCBSplineLoop sl;
		generateKnots(i, sl);
		generateSplines(sl);
		loops[i] = sl;
	}
}

void CCBSplines::generateKnots(int mod, CCBSplineLoop& sl) {
	for (int i = 0; i < CSNK; ++i) {
		sl.knots[i] = Vector3((rand() * (1.f / RAND_MAX))*5.f-2.5f, (rand() * (1.f / RAND_MAX))*3.5f-1.75f, (rand() * (1.f / RAND_MAX))*5.f-2.5f);
	}
}

void CCBSplines::generateSplines(CCBSplineLoop& sl) {
	for (int c = 0; c < CSNK; ++c){
		for (int i = 0; i < CSLOD; ++i) {
			float t = (float)i / (float)CSLOD;
			float it= (float)1.0-t;

			float b0 = it*it*it/6.f;
			float b1 = (3.f*t*t*t-6.f*t*t+4.f)/6.f;
			float b2 = (-3.f*t*t*t+3.f*t*t+3.f*t+1)/6.f;
			float b3 = t*t*t/6.f;
	
			float x = b0*sl.knots[(c-0)%CSNK].x + b1*sl.knots[(c+1)%CSNK].x + b2*sl.knots[(c+2)%CSNK].x + b3*sl.knots[(c+3)%CSNK].x;
			float y = b0*sl.knots[(c-0)%CSNK].y + b1*sl.knots[(c+1)%CSNK].y + b2*sl.knots[(c+2)%CSNK].y + b3*sl.knots[(c+3)%CSNK].y;
			float z = b0*sl.knots[(c-0)%CSNK].z + b1*sl.knots[(c+1)%CSNK].z + b2*sl.knots[(c+2)%CSNK].z + b3*sl.knots[(c+3)%CSNK].z;

			sl.vectors[c*CSLOD+i] = Vector3(x,y,z);
		}
	}
}



