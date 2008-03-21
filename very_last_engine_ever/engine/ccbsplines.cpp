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
		drawFrame(effect, time);
		effect->EndPass();
	}
	effect->End();
}

void CCBSplines::drawFrame(engine::Effect &effect, double stime) {
	int time = int(floor(stime * 8));
	for (int i = 0; i < CSLOOP; ++i) {
		float size =  0.75f + notRandf(i) * 0.25f;
		size *= 0.25f;
		ps.begin();
		for (int c = time; c < time+CSLOD; ++c)
		{
			ps.add(loops[i].vectors[c%(CSNK*CSLOD)], size / (1 + ((time + CSLOD) - c) * 0.125f));
		}
		ps.end();
		ps.draw();
	}
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
		sl.knots[i] = Vector3((rand() * (1.f / RAND_MAX))*5.f-2.5f,(rand() * (1.f / RAND_MAX))*5.f-2.5f,(rand() * (1.f / RAND_MAX))*5.f-2.5f);
		//sl.knots[i] = Vector3(notRandf(i+mod+1)*5.f-2.5f,notRandf(i+mod+2)*5.f-2.5f,notRandf(i+mod+3)*5.f-2.5f);
	}
}

Vector3 evalSpline(const Vector3 &v0, const Vector3 &v1, const Vector3 &v2, const Vector3 &v3, float t)
{
	float it= (float)1.0-t;

	float b0 = it*it*it/6.f;
	float b1 = (3.f*t*t*t-6.f*t*t+4.f)/6.f;
	float b2 = (-3.f*t*t*t+3.f*t*t+3.f*t+1)/6.f;
	float b3 = t*t*t/6.f;

	float x = b0*v0.x + b1*v1.x + b2*v2.x + b3*v3.x;
	float y = b0*v0.y + b1*v1.y + b2*v2.y + b3*v3.y;
	float z = b0*v0.z + b1*v1.z + b2*v2.z + b3*v3.z;
	return Vector3(x, y, z);
}

void CCBSplines::generateSplines(CCBSplineLoop& sl) {
	for (int c = 0; c < CSNK; ++c){
		for (int i = 0; i < CSLOD; ++i) {
			float t = (float)i / (float)CSLOD;
			sl.vectors[c*CSLOD+i] = evalSpline(sl.knots[(c-0)%CSNK], sl.knots[(c+1)%CSNK], sl.knots[(c+2)%CSNK], sl.knots[(c+3)%CSNK], t);
		}
	}
}



