#include "stdafx.h"
#include "Grow.h"
#include "../core/err.h"
#include "../math/notrand.h"
using engine::Grow;
using namespace math;
#include <algorithm>
using namespace std;


void Grow::draw(engine::Effect &effect, float time, int part) {
	if (part == 0) return;
	if (part >= 1 && st0 == 0) st0 = time;
	if (part >= 3 && st2 == 0) st2 = time;
	if (part >= 4 && st3 == 0) st3 = time;
	if (part >= 5 && st4 == 0) st4 = time;
	if (part >= 6 && st5 == 0) st5 = time;
	if (part >= 7 && st6 == 0) st6 = time;
	UINT passes;
	effect->Begin(&passes, 0);
	for (UINT pass = 0; pass < passes; ++pass)
	{
		effect->BeginPass( pass );
		drawFrame(effect,time,part);
		effect->EndPass();
	}
	effect->End();
}

void Grow::drawFrame(engine::Effect &effect, float time, int part) {
	if (st0 != 0) {
		float c0 = min(time-st0,(float)SSYNCMAX) / (float)SSYNCMAX;
		int it0 = (int)(((SNK-3)*SLOD)*c0);

		for (int i = 0; i < SCLUSTER*2-1; ++i) {
			vs.begin(D3DPT_LINELIST);
			for (int c = 0; c < it0-1; ++c) {
				vs.vertex(*loops[i]->vectors[c%(SNK*SLOD)]);
				vs.vertex(*loops[i]->vectors[(c+1)%(SNK*SLOD)]);
			}
			vs.end();
		}
	}
	if (st2 != 0) {
		float c2 = min(time-st2,(float)SSYNCMAX) / (float)SSYNCMAX;
		int it2 = (int)(((SNK-3)*SLOD)*c2);
		for (int i = SCLUSTER*2; i < SCLUSTER*3-1; ++i) {
			vs.begin(D3DPT_LINELIST);
			for (int c = 0; c < it2-1; ++c) {
				vs.vertex(*loops[i]->vectors[c%(SNK*SLOD)]);
				vs.vertex(*loops[i]->vectors[(c+1)%(SNK*SLOD)]);
			}
			vs.end();
		}
	}
	if (st3 != 0) {
		float c3 = min(time-st3,(float)SSYNCMAX) / (float)SSYNCMAX;
		int it3 = (int)(((SNK-3)*SLOD)*c3);
		for (int i = SCLUSTER*3; i < SCLUSTER*4-1; ++i) {
			vs.begin(D3DPT_LINELIST);
			for (int c = 0; c < it3-1; ++c) {
				vs.vertex(*loops[i]->vectors[c%(SNK*SLOD)]);
				vs.vertex(*loops[i]->vectors[(c+1)%(SNK*SLOD)]);
			}
			vs.end();
		}
	}
	if (st4 != 0) {
		float c4 = min(time-st4,(float)SSYNCMAX) / (float)SSYNCMAX;
		int it4 = (int)(((SNK-3)*SLOD)*c4);
		for (int i = SCLUSTER*4; i < SCLUSTER*5-1; ++i) {
			vs.begin(D3DPT_LINELIST);
			for (int c = 0; c < it4-1; ++c) {
				vs.vertex(*loops[i]->vectors[c%(SNK*SLOD)]);
				vs.vertex(*loops[i]->vectors[(c+1)%(SNK*SLOD)]);
			}
			vs.end();
		}
	}
	if (st5 != 0) {
		float c5 = min(time-st5,(float)SSYNCMAX) / (float)SSYNCMAX;
		int it5 = (int)(((SNK-3)*SLOD)*c5);
		for (int i = SCLUSTER*5; i < SCLUSTER*6-1; ++i) {
			vs.begin(D3DPT_LINELIST);
			for (int c = 0; c < it5-1; ++c) {
				vs.vertex(*loops[i]->vectors[c%(SNK*SLOD)]);
				vs.vertex(*loops[i]->vectors[(c+1)%(SNK*SLOD)]);
			}
			vs.end();
		}
	}
	if (st6 != 0) {
		float c6 = min(time-st6,(float)SSYNCMAX) / (float)SSYNCMAX;
		int it6 = (int)(((SNK-3)*SLOD)*c6);
		for (int i = SCLUSTER*6; i < SCLUSTER*7-1; ++i) {
			vs.begin(D3DPT_LINELIST);
			for (int c = 0; c < it6-1; ++c) {
				vs.vertex(*loops[i]->vectors[c%(SNK*SLOD)]);
				vs.vertex(*loops[i]->vectors[(c+1)%(SNK*SLOD)]);
			}
			vs.end();
		}
	}
	
	
	
/*
		int frame = (int)(((SNK)*SLOD*SLERPFACTOR)*completeness);
		int step = (int)(frame/it);
		Vector3 v1 = *loops[i]->vectors[it-1%(SNK*SLOD)];
		Vector3 v2 = *loops[i]->vectors[it%(SNK*SLOD)];
		Vector3 newend;
		float factor = (1/SLERPFACTOR)*(step%SLERPFACTOR+1);
		D3DXVec3Lerp(&newend, &v1, &v2, factor);

		vs.vertex(v1);
		vs.vertex(newend);
*/
}



void Grow::generateSplineLoops() {
	st0 = 0;
	st2 = 0;
	st3 = 0;
	st4 = 0;
	st5 = 0;
	st6 = 0;
/*
	Vector3(-3.8f,-2.f,0.f) 
	Vector3(3.8f,-2.f,-1.0.f)
*/
	int c = 0;

	for (int i = 0; i < SLOOP; ++i){
		CCBSplineLoop* sl = new CCBSplineLoop();

 		float lerpmod = (1.f/(SLOOP))*c;
		if (i%SCLUSTER == SCLUSTER-1) c += SCLUSTER;



		Vector3 root;
		Vector3 heading;
		if (c == 0) {
			float r = (rand() * (1.f / RAND_MAX));
			root    = Vector3(start.x-r    , start.y-r, start.z);
			heading = Vector3(start.x+3.f+r, start.y+5.f, start.z-r*5);
		} else if (c == 1*SCLUSTER) {
			float r = (rand() * (1.f / RAND_MAX));
			root    = Vector3(start.x+6.f+r, start.y-r    , start.z);
			heading = Vector3(start.x+5.f-r, start.y+5.f, start.z-r*5);
		} else if (c == 2*SCLUSTER) {
			float r = (rand() * (1.f / RAND_MAX));
			root    = Vector3(start.x+3.f, start.y-3.f, start.z-1.f);
			heading = Vector3(start.x+4.f, start.y+5.f, start.z-3.f);
		} else if (c == 3*SCLUSTER) {
			float r = (rand() * (1.f / RAND_MAX));
			root    = Vector3(start.x+2.f, start.y, start.z+3.f);
			heading = Vector3(start.x+4.f, start.y+6.f, start.z-8.f);
		} else if (c == 4*SCLUSTER) {
			float r = (rand() * (1.f / RAND_MAX));
			root    = Vector3(start.x+1.f, start.y-3*r, start.z-1.f);
			heading = Vector3(start.x+4.f, start.y+8.f, start.z+4.f);
		} else if (c == 5*SCLUSTER) {
			float r = (rand() * (1.f / RAND_MAX));
			root    = Vector3(start.x+4.f, start.y    , start.z+3.f);
			heading = Vector3(start.x+4.f, start.y+4.f+r, start.z-4.f);
		} else {
			float r = (rand() * (1.f / RAND_MAX));
			root    = Vector3(start.x+3.f, start.y-1.f    , start.z+1.f);
			heading = Vector3(start.x+6.5f, start.y+5.f, start.z+3.f);
		}

		root.x += cos (rand() * (1.f / RAND_MAX));
		root.z += sin (rand() * (1.f / RAND_MAX));
		root.y -= root.z;
		generateKnots(root, heading, *sl);
		loops[i] = sl;
	}


	for (int i = 0; i < SLOOP; ++i)
		generateSplines(*loops[i]);
}

void Grow::generateKnots(Vector3& root, Vector3& heading,CCBSplineLoop& sl) {
	for (int i = 0; i < SNK; ++i) {
		Vector3 vec;
		D3DXVec3Lerp(&vec, &root, &heading, ((float)i/(float)SNK));
		sl.knots[i] = new Vector3(vec.x+(rand() * (1.f / RAND_MAX))*0.1f, vec.y, vec.z+(rand() * (1.f / RAND_MAX)));

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



