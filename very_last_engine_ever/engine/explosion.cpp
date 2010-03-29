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
	rotCounter = 0;
	lastTime = 0;
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
	Matrix4x4 mtrans;
	for (int i = 0; i < count; ++i) {
		if (i <EXPLOSION_INIT_TRIANGLE_COUNT ) {
			Vector3 axis = cross(end-begin, data->dir);
			D3DXMatrixRotationAxis(&mrot, &axis, rotCounter/data->weight*0.1f);
			for (int j = 0; j < 3; ++j) data[j].pos      = mul(mrot, data[j].initPos);
			Vector3 norm = normalize(cross(data[1].pos - data[0].pos, data[2].pos - data[0].pos));
			for (int j = 0; j < 3; ++j) data[j].norm = norm;
			data += 3;
		} else {
			D3DXMatrixRotationAxis(&mrot, &data->dir, rotCounter/data->weight*0.02f);
			
			if (lastTime-time != 0) data->pos      = mul(mrot, data->initPos);
			data++;

			if (lastTime-time != 0) data->pos      = mul(mrot, data->initPos);
			data++;

			if (lastTime-time != 0) data->pos      = mul(mrot, data->initPos);
			data++;
		}
	}
	rotCounter++;
/*
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

		Vector3 avg = (pos1 + pos2 + pos3)/3;
		pos1 -= avg;
		pos2 -= avg;
		pos3 -= avg;

		float weight   = 0.5 * math::length(cross(pos3 - pos1, pos2 - pos1)); 
		float size	   = notRandf(time+3);

		Matrix4x4 minitrot;
		minitrot.makeRotation(Vector3(D3DXToRadian(360*notRandf(time+i+5)),D3DXToRadian(360*notRandf(time+i+6)),D3DXToRadian(360*notRandf(time+i+7))));

		data->uv       = uv1;
		data->pos      = mul(minitrot,pos1);
		data->initPos  = mul(minitrot,pos1);
		data->dir	   = end-begin;
		data->index    = (float) (i-EXPLOSION_INIT_TRIANGLE_COUNT);
		data->size     = size;
		data->weight   = weight;
		data++;

		data->uv       = uv2;
		data->pos      = mul(minitrot,pos2);
		data->initPos  = mul(minitrot,pos2);
		data->dir	   = end-begin;
		data->index    = (float) (i-EXPLOSION_INIT_TRIANGLE_COUNT);
		data->size     = size;
		data->weight   = weight;
		data++;

		data->uv       = uv3;
		data->pos      = mul(minitrot,pos3);
		data->initPos  = mul(minitrot,pos3);
		data->dir	   = end-begin;
		data->index    = (float) (i-EXPLOSION_INIT_TRIANGLE_COUNT);
		data->size     = size;
		data->weight   = weight;
		data++;
	}
	count = countStop;
*/

	vb.unlock();
	data = NULL;
	lastTime = time;
}

void Explosion::generateGraphics() {
	count = EXPLOSION_INIT_TRIANGLE_COUNT;
	ExplosionVertex* data = NULL;
	data = (ExplosionVertex*) vb.lock(0, sizeof(ExplosionVertex) * EXPLOSION_BUFFER_SIZE, D3DLOCK_DISCARD);
	assert(NULL != data);

	for (int i = 0; i < count; ++i) {
		float s = 0;
		s			   = notRandf(i)*EXPLISION_FRAGMENT_FACTOR;
		Vector2 uv1	   = Vector2(0,0);
		Vector3 pos1   = Vector3(-s,-s,0);

		s			   = notRandf(i+1)*EXPLISION_FRAGMENT_FACTOR;
		Vector2 uv2	   = Vector2(0,1);
		Vector3 pos2   = Vector3(-s,s,0);

		s			   = notRandf(i+2)*EXPLISION_FRAGMENT_FACTOR;
		Vector2 uv3	   = Vector2(1,1);
		Vector3 pos3   = Vector3(s,s,0);
		
		float weight   = 0.5f * math::length(cross(pos3 - pos1, pos2 - pos1)); 
		float size	   = notRandf(i+3);

		Vector3 avg = (pos1 + pos2 + pos3)/3;
		pos1 -= avg;
		pos2 -= avg;
		pos3 -= avg;

		Vector3 dir    = end - begin;
		Vector3 axisX  = normalize(findOrthogonal(dir));
		Vector3 axisY  = normalize(cross(dir, axisX));
		float r        = EXPLOSION_MAX_RADIUS * pow(notRandf(i+1), EXPLOSION_CONE_FACTOR);
		float theta    = (float)(2.0f*M_PI*notRandf(i+2)); 
		float z        = 1.0f - sqrt(notRandf(i+3));
		Vector3 newend = r*z*cos(theta) * axisX + r*z*sin(theta) * axisY + z*dir + begin;
		//Vector3 newend = Vector3(begin.x+4.0f,begin.y,begin.z);


		Matrix4x4 mrotz;
		mrotz.makeRotation(Vector3(D3DXToRadian(360*notRandf(i+5)),D3DXToRadian(360*notRandf(i+6)),D3DXToRadian(360*notRandf(i+7))));

		data->uv       = uv1;
		data->pos      = mul(mrotz,pos1);
		data->initPos  = mul(mrotz,pos1);
		data->dir	   = newend-begin;
		data->index    = (float) 0;
		data->size     = size;
		data->weight   = weight;
		data++;

		data->uv       = uv2;
		data->pos      = mul(mrotz,pos2);
		data->initPos  = mul(mrotz,pos2);
		data->dir	   = newend-begin;
		data->index    = (float) 0;
		data->size     = size;
		data->weight   = weight;
		data++;

		data->uv       = uv3;
		data->pos      = mul(mrotz,pos3);
		data->initPos  = mul(mrotz,pos3);
		data->dir	   = newend-begin;
		data->index    = (float) 0;
		data->size     = size;
		data->weight   = weight;
		data++;
	}

	vb.unlock();
	data = NULL;
}
