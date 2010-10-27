#include "stdafx.h"
#include "triangleeffect.h"
#include "../core/err.h"
#include "../math/matrix4x4.h"
#include "../math/vector3.h"

using engine::TriangleEffect;
using math::Vector3;
using math::Matrix4x4;


void TriangleEffect::draw(engine::Effect &effect, engine::VertexStreamer &streamer, double beat, unsigned numOfTris, float size, float dist, float shaper, float opening)
{
	if (numOfTris > 0) {
		//modifiables
/*		float size = 4.f;
		float space = 7.f;
		float dist = 0.75f;
		float shaper = 0.f;
*/

		float space = size*1.25f;

		unsigned circle_count = 30;
		float circle_steps = 360.f / circle_count;
		float spiral_steps = 360.f / (numOfTris / circle_count);


		//feeding the shader
		Matrix4x4 mrot2;
		mrot2.rotation(Vector3(float(-M_PI / 4), float(M_PI - sin(beat / 6)), float(M_PI + beat / 4)));

		effect->SetMatrix(effect->GetParameterBySemantic(0, "ROTATION"), &mrot2);
		float dir_vec[2];
		dir_vec[0] = cosf(D3DXToRadian((unsigned)beat/60%360)) * (1.0f / 30);
		dir_vec[1] = sinf(D3DXToRadian((unsigned)beat/60%360)) * (1.0f / 30);

		effect->SetFloatArray("dir", dir_vec, 2);


		UINT passes;
		effect->Begin(&passes, 0);
		for (UINT pass = 0; pass < passes; ++pass)
		{
			effect->BeginPass( pass );
			streamer.begin(D3DPT_TRIANGLELIST);


			
			for (unsigned i = 0; i < circle_count;  ++i)
			{
				float c_rad = D3DXToRadian(circle_steps*i);
				float c_cos = cosf(c_rad)*(2*space*circle_count);
				float c_sin = sinf(c_rad)*(2*space*circle_count);

				for (unsigned j = 0; j < (unsigned)(numOfTris/(double)circle_count+0.5f); ++j) {
					float doff = (rand() * (1.f / RAND_MAX) + 0.5f)*dist;
					float s_rad = D3DXToRadian(spiral_steps*j);
					float s_cos = (s_rad*opening)*cosf(6*s_rad+c_rad)*space;
					float s_sin = (s_rad*opening)*sinf(6*s_rad+c_rad)*space;

					Vector3 pos = Vector3(s_cos,s_sin,j*space);
					Vector3 shaper_vec = pos*shaper;

					Matrix4x4 mrot;
//					mrot.make_rotation(Vector3(doff+((unsigned)(beat/60)%360)*M_PI/180, 0, ((unsigned)(beat/60)%360)*M_PI/180));
//					mrot.make_rotation(Vector3(float(-M_PI / 2), float(M_PI - ( beat / 3)), s_rad));
					mrot.rotation(Vector3(-D3DXToRadian(beat+s_rad), s_rad+c_rad, D3DXToRadian((rand() * (1.f / RAND_MAX) + 0.5f)+beat)));


					streamer.uv(    D3DXVECTOR2(-size, -size));
					streamer.vertex(mul(mrot,Vector3(-size+doff, -size,     0.f)+shaper_vec)+pos);
					streamer.uv(    D3DXVECTOR2(-size, size));
					streamer.vertex(mul(mrot,Vector3(-size,      size+doff, 0.f)+shaper_vec)+pos);
					streamer.uv(    D3DXVECTOR2( size, size));
					streamer.vertex(mul(mrot,Vector3( size,      size,      doff)+shaper_vec)+pos);

				}
			}



			streamer.end();
			effect->EndPass();
		}
		effect->End();
	}
}

