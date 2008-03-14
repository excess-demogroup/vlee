#include "stdafx.h"
#include "Explosion.h"
#include "../core/err.h"
#include "../math/notrand.h"
#include <algorithm>

using namespace math;
using namespace std;
using engine::Explosion;

void Explosion::setupParameters(Vector3 &begin, Vector3 &end) {
	dStep = length(begin+end)/(EXPLOSION_ANIMATION_LENGTH*4);
	this->begin = begin;
	this->end = end;
	mworld.makeTranslation(begin);
}

void Explosion::draw(engine::Effect &effect, int time) {
	if (time > 0) {
	updateGraphics(time);

		effect->SetMatrix("worldpos", &mworld);
		effect->SetFloat("dstep", dStep);
		effect->SetFloat("time", (float)time);

		UINT passes;
		effect->Begin(&passes, 0);
		for (UINT pass = 0; pass < passes; ++pass) {
			effect->BeginPass(pass);

			device->SetStreamSource(0, vb, 0, sizeof(ExplosionVertex));
			device->SetFVF(EXPLOSIONFVF);
			device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, count);

			effect->EndPass();
		}
		effect->End();
	}
}


void Explosion::updateGraphics(int time) {
	ExplosionVertex* data = NULL;
	data = (ExplosionVertex*) vb.lock(0, sizeof(ExplosionVertex) * EXPLOSION_BUFFER_SIZE, 0);
	assert(NULL != data);

	//transformation
	Matrix4x4 mrot;
	Matrix4x4 mscale;
	for (int i = 0; i < count; ++i) {
		mrot.makeRotation(Vector3(D3DXToRadian(time%2+1),D3DXToRadian(time%2+1),D3DXToRadian(time%2+1)));
		bool scaling = false;
		float newsize = 1.0f;
		if (data->size < 1.0f) {
			mscale.makeScaling(Vector3(1+(1-data->size)*dStep,1+(1-data->size)*dStep,1+(1-data->size)*dStep));
			newsize += (1-data->size)*dStep;
			scaling = true;
		}

		if (scaling) {
			data->size	   = newsize;
			data->pos      = mul(mrot, mul(mscale,data->pos));
		} else {
			data->pos      = mul(mrot, data->pos);
		}
		data++;

		if (scaling) {
			data->size	   = newsize;
			data->pos      = mul(mrot, mul(mscale,data->pos));
		} else {
			data->pos      = mul(mrot, data->pos);
		}
		data++;

		if (scaling) {
			data->size	   = newsize;
			data->pos      = mul(mrot, mul(mscale,data->pos));
		} else {
			data->pos      = mul(mrot, data->pos);
		}
		data++;
	}
	//create additional fragments
	int countStop = min(count+(unsigned)time,(unsigned)(EXPLOSION_ANIMATION_LENGTH+EXPLOSION_INIT_TRIANGLE_COUNT));
	for (int i = count; i < countStop; ++i) {
		float s = 0;
		s			   = notRandf(time+i+1);
		Vector2 uv1	   = Vector2(0,0);
		Vector3 pos1   = Vector3(-s,-s,0);

		s			   = notRandf(time+i+1);
		Vector2 uv2	   = Vector2(0,1);
		Vector3 pos2   = Vector3(-s,s,0);

		s			   = notRandf(time+i+2);
		Vector2 uv3	   = Vector2(1,1);
		Vector3 pos3   = Vector3(s,s,0);

		float weight   = math::length(pos1)+math::length(pos2)+math::length(pos3); 
		float size	   = notRandf(time+3);

		Matrix4x4 minitrot;
		minitrot.makeRotation(Vector3(D3DXToRadian(360*notRandf(time+i+5)),D3DXToRadian(360*notRandf(time+i+6)),D3DXToRadian(360*notRandf(time+i+7))));

		data->uv       = uv1;
		data->pos      = mul(minitrot,pos1);
		data->dir	   = (end*(dStep*(i-EXPLOSION_INIT_TRIANGLE_COUNT)))-begin;
		data->index    = (float) (i-EXPLOSION_INIT_TRIANGLE_COUNT);
		data->size     = size;
		data->weight   = weight;
		data++;

		data->uv       = uv2;
		data->pos      = mul(minitrot,pos2);
		data->dir	   = (end*(dStep*(i-EXPLOSION_INIT_TRIANGLE_COUNT)))-begin;
		data->index    = (float) (i-EXPLOSION_INIT_TRIANGLE_COUNT);
		data->size     = size;
		data->weight   = weight;
		data++;

		data->uv       = uv3;
		data->pos      = mul(minitrot,pos3);
		data->dir	   = (end*(dStep*(i-EXPLOSION_INIT_TRIANGLE_COUNT)))-begin;
		data->index    = (float) (i-EXPLOSION_INIT_TRIANGLE_COUNT);
		data->size     = size;
		data->weight   = weight;
		data++;
	}
	count = countStop;


	vb.unlock();
	data = NULL;
}

void Explosion::generateGraphics() {
	count = EXPLOSION_INIT_TRIANGLE_COUNT;
	ExplosionVertex* data = NULL;
	data = (ExplosionVertex*) vb.lock(0, sizeof(ExplosionVertex) * EXPLOSION_BUFFER_SIZE, D3DLOCK_DISCARD);
	assert(NULL != data);

	for (int i = 0; i < count; ++i) {
		float s = 0;
		s			   = notRandf(i);
		Vector2 uv1	   = Vector2(0,0);
		Vector3 pos1   = Vector3(-s,-s,0);

		s			   = notRandf(i+1);
		Vector2 uv2	   = Vector2(0,1);
		Vector3 pos2   = Vector3(-s,s,0);

		s			   = notRandf(i+2);
		Vector2 uv3	   = Vector2(1,1);
		Vector3 pos3   = Vector3(s,s,0);
		
		float weight   = math::length(pos1)+math::length(pos2)+math::length(pos3); 
		float size	   = notRandf(i+3);

		Vector3 dir    = end - begin;		Vector3 axisX  = normalize(findOrthogonal(dir));		Vector3 axisY  = normalize(cross(dir, axisX));		float r        = EXPLOSION_MAX_RADIUS * pow(notRandf(i+1), EXPLOSION_CONE_FACTOR);		float theta    = (float)(2.0f*M_PI*notRandf(i+2)); 		float z        = 1.0f - sqrt(notRandf(i+3));		Vector3 newend = r*z*cos(theta) * axisX + r*z*sin(theta) * axisY + z*dir + begin;

		Matrix4x4 mrotz;
		mrotz.makeRotation(Vector3(D3DXToRadian(360*notRandf(i+5)),D3DXToRadian(360*notRandf(i+6)),D3DXToRadian(360*notRandf(i+7))));

		data->uv       = uv1;
		data->pos      = mul(mrotz,pos1);
		data->dir	   = newend-begin;
		data->index    = (float) 0;
		data->size     = size;
		data->weight   = weight;
		data++;

		data->uv       = uv2;
		data->pos      = mul(mrotz,pos2);
		data->dir	   = newend-begin;
		data->index    = (float) 0;
		data->size     = size;
		data->weight   = weight;
		data++;

		data->uv       = uv3;
		data->pos      = mul(mrotz,pos3);
		data->dir	   = newend-begin;
		data->index    = (float) 0;
		data->size     = size;
		data->weight   = weight;
		data++;
	}

	vb.unlock();
	data = NULL;
}
