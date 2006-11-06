#ifndef MEGADEMO_H
#define MEGADEMO_H

#define END_TIME (60 * 3 + 30) /* 3:30 */

#include "renderer/texture.h"
#include "renderer/rendertexture.h"
#include "renderer/device.h"
#include "engine/demo.h"
#include "engine/image.h"
#include "engine/particlestreamer.h"
#include "math/vector3.h"
#include "math/vector2.h"
#include "math/matrix4x4.h"

using renderer::Texture;
using renderer::RenderTexture;
using engine::Image;
using engine::Mesh;

using math::Vector3;
using math::Vector2;
using math::Matrix4x4;

namespace engine
{
	Texture load_texture(renderer::Device &device, std::string filename)
	{
		Texture tex;

		HRESULT hr = D3DXCreateTextureFromFile(device, filename.c_str(), &tex);
		if (FAILED(hr)) throw core::FatalException(std::string("failed to load mesh \"") + filename + std::string("\"\n\n") + core::d3d_get_error(hr));

		return tex;
	}
}

void set_ramp(float alphas[], float base)
{
	for (unsigned i = 0; i < 8; i++)
	{
		alphas[i] = base;
		base *= 0.5f;
	}
}

void blit(IDirect3DDevice9 *device, IDirect3DTexture9 *tex, Effect &eff, Mesh &polygon)
{
	assert(polygon);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	eff->SetTexture("tex", tex);
	eff->CommitChanges();
	eff.draw(polygon);
}

template <typename T>
void insertion_sort(T* a, size_t len)
{
	for (int j = 2; j < len; j++)
	{
		for (int k = 0; k < j; k++)
		{
			if (a[j] < a[k])
			{
				temp = a[k];
				a[k] = a[j];
				a[j] = temp;
			}
		}
	}
}

#if 0
typedef D3DXVECTOR2 Vector2;
/* typedef D3DXVECTOR3 Vector3; */
typedef D3DXVECTOR4 Vector4;
#endif

class Particle
{
public:
	Particle(const Vector3 &pos, const float size) : pos(pos), size(size) {}

	Vector3 pos;
	float size;
};


#include <list>

class ParticleCompare
{
public:
	ParticleCompare(const Vector3 &dir) : dir(dir) {}

	bool operator()(const Particle &a, const Particle &b)
	{
		float za = dir.x * a.pos.x + dir.y * a.pos.y + dir.z * a.pos.z;
		float zb = dir.x * b.pos.x + dir.y * b.pos.y + dir.z * b.pos.z;
		if (fabs(za - zb) > 1e-5) return za > zb;
		else return false;
	}

	const Vector3 &dir;
};

#include <algorithm>

class ParticleCloud
{
public:
	ParticleCloud() : streamer(NULL) {}
	ParticleCloud(engine::ParticleStreamer *streamer) : streamer(streamer) {}
	
	void sort(const Vector3 &dir)
	{
		particles.sort(ParticleCompare(dir));
	}
	
	std::list<Particle> particles;
	engine::ParticleStreamer *streamer;
};

Matrix4x4 texture_matrix(const Texture &tex)
{
	Matrix4x4 mat, trans1, trans2, scale;
	D3DXMatrixIdentity(&trans1);
	trans1._31 = 0.5 / tex.get_surface().get_desc().Width;
	trans1._32 = 0.5 / tex.get_surface().get_desc().Height;

	D3DXMatrixIdentity(&trans2);
	trans1._31 = 0.5;
	trans1._32 = 0.5;

	D3DXMatrixScaling(&scale, 0.5, 0.5, 1.0);
	mat = scale * trans2;

//	mat = trans1 * scale;
	return mat;
}

Matrix4x4 radialblur_matrix(const Texture &tex, const Vector2 &center, const float amt = 1.01)
{
	Matrix4x4 mat;
	mat.make_scaling(Vector3(amt, amt, 1.0));
#if 1
	mat._31 = 0.005;
	mat._32 = 0.005;
#endif
	return mat;
}

class MegaDemo : public engine::Demo
{
private:
	float aspect;

	Mesh polygon;
	Mesh mesh2;

	Mesh    tunelle_mesh;
	Texture tunelle_tex;

	Effect test_fx;
	Effect blur_fx;
	Effect tex_fx;
	Effect tunelle_fx;
	Effect particle_fx;

	Texture logo_tex;
	Image logo;

	RenderTexture blurme1_tex;
	Image blurme1;

	RenderTexture blurme2_tex;
	Image blurme2;

	Texture bartikkel_tex;

//	engine::TextureProxy texloader;

/*	engine::bass::Stream music;
	engine::core::Texture tex;
	engine::scenegraph::Scene scene; */

	Sync &sync;
	SyncTrack &fade, &flash, &part;
	SyncTrack &xrot, &yrot, &zrot;
	SyncTrack &cam_seed, &cam_rand;
	SyncTrack &cam_x, &cam_y, &cam_z;
	SyncTrack &cam_fov;
	SyncTrack &blur_amt;

	engine::ParticleStreamer streamer;
	ParticleCloud cloud;

public:
	MegaDemo(renderer::Device &device, renderer::Surface &backbuffer, float aspect, Sync &sync) :
		Demo(device, backbuffer),
		aspect(aspect),
		sync(sync),
		fade( sync.getTrack("fade",     "global", 5, true)),
		flash(sync.getTrack("flash",    "global", 5, true)),
		part( sync.getTrack("part",     "global", 5, true)),
		blur_amt( sync.getTrack("blur",     "global", 5, true)),
		xrot( sync.getTrack("x",        "rotation", 5, true)),
		yrot( sync.getTrack("y",        "rotation", 5, true)),
		zrot( sync.getTrack("z",        "rotation", 5, true)),

		cam_x( sync.getTrack("x",       "cam", 5, true)),
		cam_y( sync.getTrack("y",       "cam", 5, true)),
		cam_z( sync.getTrack("z",       "cam", 5, true)),
		cam_fov( sync.getTrack("fov",   "cam", 5, true)),
		cam_seed( sync.getTrack("seed", "cam", 5, true)),
		cam_rand( sync.getTrack("rand", "cam", 5, true))
	{
		streamer = engine::ParticleStreamer(device);
		
/*		tex   = texloader.get("test.jpg");
		scene = sceneloader.get("test.scene");
		music = musicloader.get(); */
		
		d3d_err(D3DXCreatePolygon(device, 3.f, 4, &polygon, 0));
//		mesh2 = engine::load_mesh(device, "data/test.x");

		tunelle_mesh = engine::load_mesh(device, "data/tunelle.x");
		tunelle_tex  = engine::load_texture(device, "data/tunelle.dds");

		test_fx     = engine::load_effect(device, "data/test.fx");
		blur_fx     = engine::load_effect(device, "data/blur.fx");
		tex_fx      = engine::load_effect(device, "data/tex.fx");
		particle_fx = engine::load_effect(device, "data/particle.fx");
		tunelle_fx  = engine::load_effect(device, "data/tunelle.fx");
		
		logo_tex = engine::load_texture(device, "data/logo.png");
		logo = Image(logo_tex, tex_fx, polygon);
		
		bartikkel_tex = engine::load_texture(device, "data/bartikkel.dds");
		
		CComPtr<IDirect3DCubeTexture9> env;
		d3d_err(D3DXCreateCubeTextureFromFile(device, "data/stpeters_cross2.dds", &env));
		test_fx->SetTexture("env", env);
		
		blurme1_tex = RenderTexture(device, backbuffer.get_desc().Width / 2, int(backbuffer.get_desc().Height * aspect * (9 / 16.f)) / 2);
		blurme2_tex = RenderTexture(device, backbuffer.get_desc().Width / 2, int(backbuffer.get_desc().Height * aspect * (9 / 16.f)) / 2);

		for (int i = 0; i < 1024 * 16; ++i)
		{
#if 1
			float x = (randf() - 0.5f) * 200;
			float z = (randf() - 0.5f) * 200;
			float y = (randf() - 0.5f) * 200;
			float s = (randf() + 0.5f) * 4.f;
#else
			float p = i;
			float rad = 50 + sin(p * 0.0001f) * 10;
			float x = (sin(p * 0.01)) * rad;
			float z = (cos(p * 0.01)) * rad;
			float y = (sin(p * 0.032)) * 10;
			float s = 3.f + sin(p * 0.189) * 2;
			s *= 0.5;
#endif
			cloud.particles.insert(cloud.particles.end(), Particle(Vector3(x, y, z), s));
		}
	}

	void start()
	{
/*		music.play(); */
	}

	void draw(float time)
	{
		double beat = time * (double(BPM) / 60);

#ifdef VJSYS
		// is there any way of recieving _all_ fft-data without recording?
		while (BASS_ChannelGetData(stream, 0, BASS_DATA_AVAILABLE) >= 2048 * 2)
		{
			BASS_ChannelGetData(stream, spectrum, BASS_DATA_FFT512);
		}
#endif
		if (BASS_ChannelGetData(stream, 0, BASS_DATA_AVAILABLE) >= 2048)
		{
			BASS_ChannelGetData(stream, spectrum, BASS_DATA_FFT512);
		}

		if (true)
		{
			device->SetRenderTarget(0, blurme1_tex.get_surface());
			device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xffffff, 1.f, 0);

			// setup projection
			D3DXMATRIX proj;

			D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(cam_fov.getFloatValue()), 16.f / 9, 0.01f, 1000.f);
			device->SetTransform(D3DTS_PROJECTION, &proj);

			float rot = time * 0.4f;

	//		srand(cam_seed.getIntValue());
			srand(int(beat));

			// setup camera (animate parameters)

			srand(0);
			D3DXVECTOR3 at(0, 0.f, 10.f);
			D3DXVECTOR3 up(0.f, 1.f, 0.f);
			D3DXVECTOR3 eye(
				cam_x.getFloatValue(), // cam_rand.getFloatValue(),
				cam_y.getFloatValue(), // cam_rand.getFloatValue(),
				cam_z.getFloatValue()
				); // cam_rand.getFloatValue(),

			// setup camera (view matrix)
			D3DXMATRIX view;
			D3DXMatrixLookAtLH(&view, &eye, &at, &up);
			device->SetTransform(D3DTS_VIEW, &view);

			// setup object matrix
			Matrix4x4 world;
			D3DXMatrixIdentity(&world);
			device->SetTransform(D3DTS_WORLD, &world);
/*
			test_fx.set_matrices(world, view, proj);
			test_fx->SetTexture("map", logo_tex);
			test_fx->SetFloat("overbright", 1.0 + 1.0 / fmod(beat, 1.0));
			test_fx->CommitChanges();
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			test_fx.draw(mesh2);
*/

			if (beat > (64 + 16))
			{
				for (int i = 0; i < 10; ++i)
				{
					Vector3 scale(0.1, 0.1, 0.1);
					Vector3 translation(0,0,i * 25);
					D3DXQUATERNION rotation;
					D3DXQuaternionRotationYawPitchRoll(&rotation, 0, M_PI / 2, 0);
					D3DXMatrixTransformation(&world, NULL, NULL, &scale, NULL, &rotation, &translation);

					tunelle_fx.set_matrices(world, view, proj);
					tunelle_fx->SetTexture("map", tunelle_tex);
					tunelle_fx->SetFloat("overbright", 1.0);
					tunelle_fx->CommitChanges();
					device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
					tunelle_fx.draw(tunelle_mesh);
				}
			}

			world.make_identity();

			Matrix4x4 modelview = world * view;
			cloud.sort(Vector3(modelview._13, modelview._23, modelview._33));

			particle_fx.set_matrices(world, view, proj);
			particle_fx->SetTexture("tex", bartikkel_tex);

			{
				Vector3 up(modelview._12, modelview._22, modelview._32);
				Vector3 left(modelview._11, modelview._21, modelview._31);
				up.normalize();
				left.normalize();

				particle_fx->SetFloatArray("up", up, 3);
				particle_fx->SetFloatArray("left", left, 3);
				particle_fx->CommitChanges();
			}
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
			device->SetRenderState(D3DRS_ZWRITEENABLE, false);

			std::list<Particle>::const_iterator iter = cloud.particles.begin();
			bool iter_done = false;

			int cnt = 0;
			while (!iter_done)
			{
				streamer.begin();
				for (int i = 0; i < 1023; ++i)
				{
					cnt++;
					if (cloud.particles.end() == iter)
					{
						iter_done = true;
						break;
					}

					streamer.add(
						Vector3(iter->pos.x,
								iter->pos.y,
								iter->pos.z
						),
#if 0
						2.f + sin(cnt * 0.00589)
#else
						iter->size + (1 - fmod(beat, 1.0)) * 2
#endif
					);
					++iter;
				}
				streamer.end();
				particle_fx.draw(streamer);
			}

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
			device->SetRenderState(D3DRS_ZWRITEENABLE, true);


			Matrix4x4 texcoord_transform;
			Matrix4x4 texel_transform;
			Matrix4x4 texture_transform = texture_matrix(blurme1_tex);

//			float amt = 1.0 - pow((1.0 - fmod(beat / 2, 1.0)) * 0.1, 2.0);
			float amt = 1.0 - pow((blur_amt.getFloatValue()) * 0.1, 2.0);

			texcoord_transform.make_scaling(Vector3(1,1,1));
			blur_fx->SetMatrix("texcoord_transform", &texcoord_transform);
			blur_fx->SetMatrix("texture_transform", &texture_transform);
#if 1
			device->SetRenderTarget(0, blurme2_tex.get_surface());
			texel_transform = radialblur_matrix(blurme1_tex, Vector2(0, 0), amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);
			blit(device, blurme1_tex, blur_fx, polygon);
			amt *= amt;

			device->SetRenderTarget(0, blurme1_tex.get_surface());
			texel_transform = radialblur_matrix(blurme2_tex, Vector2(0, 0), amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);
			blit(device, blurme2_tex, blur_fx, polygon);
			amt *= amt;

			device->SetRenderTarget(0, blurme2_tex.get_surface());
			texel_transform = radialblur_matrix(blurme1_tex, Vector2(0, 0), amt);
			blur_fx->SetMatrix("texel_transform", &texel_transform);
			blit(device, blurme1_tex, blur_fx, polygon);
			amt *= amt;
#endif
		}
#if 1
		device.set_render_target(backbuffer);
		D3DVIEWPORT9 viewport;
		device->GetViewport(&viewport);
		const float correction = (aspect * (9 / 16.f));
		int old_height = viewport.Height;
		int new_height = int(viewport.Height * correction);
		viewport.Y      = (old_height - new_height) / 2;
		viewport.Height = new_height;

		device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DXCOLOR(0.f, 0.f, 0.f, 0.f), 1.f, 0);
		device->SetViewport(&viewport);

		blit(device, blurme2_tex, blur_fx, polygon);
//		blur.draw(device);

		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		Matrix4x4 tex_transform;
		tex_transform.make_scaling(Vector3(pow(1.0 - fmod(beat / 4, 1.0), 8.0) * cos(beat * M_PI * 4) * 0.25 + 1.0, 1, 1));
		tex_fx->SetMatrix("tex_transform", &tex_transform);
		logo.draw(device);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
#endif
		
		if (time > END_TIME) done = true;
	}
};

#endif // MEGADEMO_H
