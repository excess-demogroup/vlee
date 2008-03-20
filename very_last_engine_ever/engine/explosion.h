#pragma once

#include "effect.h"
#include "../renderer/device.h"
#include "../renderer/vertexbuffer.h"
#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../math/matrix4x4.h"
using math::Vector2;
using math::Vector3;
using math::Matrix4x4;

namespace engine
{
	class Explosion {
		#define EXPLOSION_ANIMATION_LENGTH 1600
		#define EXPLOSION_INIT_TRIANGLE_COUNT 800
		#define EXPLOSION_MAX_RADIUS 15.5f
		#define EXPLOSION_CONE_FACTOR 0.5f

		#define EXPLISION_FRAGMENT_FACTOR 4.f

		#define SIZE_PERCENTAGE_RND_MIN 25
		#define SIZE_PERCENTAGE_RND_MAX 50

		#define EXPLOSION_BUFFER_SIZE (3*EXPLOSION_ANIMATION_LENGTH + 3*(EXPLOSION_INIT_TRIANGLE_COUNT))
		#define EXPLOSIONFVF (D3DFVF_XYZ | D3DFVF_TEX6 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE3(1) | \
			D3DFVF_TEXCOORDSIZE3(2) | D3DFVF_TEXCOORDSIZE1(3) | D3DFVF_TEXCOORDSIZE1(4) | D3DFVF_TEXCOORDSIZE1(5))

		struct ExplosionVertex {
			D3DXVECTOR3 pos;
			D3DXVECTOR2 uv;
			D3DXVECTOR3 dir;
			D3DXVECTOR3 initPos;
			float       index;
			float       size;
			float       weight;
		};


	public:
		Explosion(renderer::Device &device, Vector3 begin, Vector3 end) : device(device), count(0) {
			vb = device.createVertexBuffer(sizeof(ExplosionVertex) * EXPLOSION_BUFFER_SIZE, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, EXPLOSIONFVF);

			setupParameters(begin, end);
			generateGraphics();
		};

		~Explosion() {
			vb.Release();
		}

		void draw(engine::Effect &effect, int time);

	private:
		void setupParameters(Vector3 &begin, Vector3 &end);
		void updateGraphics(int time);
		void generateGraphics();

		renderer::Device &device;
		renderer::VertexBuffer vb;

		int count;
		int lastTime;
		int rotCounter;

		Vector3 begin;
		Vector3 end;

		float dStep;
		Matrix4x4 mworld;
	};
}
