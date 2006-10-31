#ifndef MEGADEMO_H
#define MEGADEMO_H

#define BPM 125
#define END_TIME (60 * 3 + 30) /* 3:30 */

#include "renderer/texture.h"
#include "renderer/rendertexture.h"
#include "renderer/device.h"
#include "engine/demo.h"
#include "engine/image.h"

using renderer::Texture;
using renderer::RenderTexture;
using engine::Image;
using engine::Mesh;

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

typedef D3DXVECTOR2 Vector2;
/* typedef D3DXVECTOR3 Vector3; */
typedef D3DXVECTOR4 Vector4;

class Vector3 : public D3DXVECTOR3
{
public:
	Vector3(double x, double y, double z) : D3DXVECTOR3(float(x), float(y), float(z))
	{

	}
};

class Matrix4x4 : public D3DXMATRIX
{
public:
	Matrix4x4() {}

	Matrix4x4(const Matrix4x4 &mat)
	{
		memcpy(this, &mat, sizeof(Matrix4x4));
	}

	Matrix4x4(const D3DXMATRIX &mat)
	{
		memcpy(this, &mat, sizeof(Matrix4x4));
	}

	void make_scale(const Vector3 &scale)
	{
		D3DXMatrixScaling(this, scale.x, scale.y, scale.z);
	}

	void make_translate(const Vector3 &translate)
	{
		D3DXMatrixTranslation(this, translate.x, translate.y, translate.z);
	}
};

class Particle
{
public:
	Vector3 pos;
};

#include <list>

class ParticleCloud
{
public:
	ParticleCloud() {}
	
	void sort(const Vector3 &dir)
	{
//		float dot = dir.dot(Vector3(1,0,0));
	}
private:
	std::list<Particle> particles;
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
	mat.make_scale(Vector3(amt, amt, 1.0));
	return mat;

	//	Matrix4x4 trans1, trans2, scale;
//	mat.make_scale(Vector3(1, 1, 1.0));
	mat._31 = 0.01;
	mat._32 = 0.01;
}

class MegaDemo : public engine::Demo
{
private:
	Device device;

	Surface backbuffer;
	float aspect;

	Mesh polygon;
	Mesh mesh2;

	Effect test_fx;
	Effect blur_fx;
	Effect tex_fx;

	Texture logo_tex;
	Image logo;

	RenderTexture blurme1_tex;
	Image blurme1;

	RenderTexture blurme2_tex;
	Image blurme2;

//	engine::TextureProxy texloader;

/*	engine::bass::Stream music;
	engine::core::Texture tex;
	engine::scenegraph::Scene scene; */

	Sync &sync;
	SyncTrack &fade, &flash, &part;
	SyncTrack &xrot, &yrot, &zrot;
	SyncTrack &cam_seed, &cam_rand;

public:
	MegaDemo(renderer::Device &device, float aspect, Sync &sync) :
		Demo(device),
		device(device),
		aspect(aspect),
		sync(sync),
		fade( sync.getTrack("fade",     "global", 5, true)),
		flash(sync.getTrack("flash",    "global", 5, true)),
		part( sync.getTrack("part",     "global", 5, true)),
		xrot( sync.getTrack("x",        "rotation", 5, true)),
		yrot( sync.getTrack("y",        "rotation", 5, true)),
		zrot( sync.getTrack("z",        "rotation", 5, true)),
		cam_seed( sync.getTrack("seed", "cam", 5, true)),
		cam_rand( sync.getTrack("rand", "cam", 5, true)),
//		texloader(device),
		backbuffer(device.get_render_target())
	{

/*		tex   = texloader.get("test.jpg");
		scene = sceneloader.get("test.scene");
		music = musicloader.get(); */



		d3d_err(D3DXCreatePolygon(device, 3.f, 4, &polygon, 0));

		mesh2 = engine::load_mesh(device, "data/test.x");

		test_fx  = engine::load_effect(device, "data/test.fx");
		blur_fx  = engine::load_effect(device, "data/blur.fx");
		tex_fx   = engine::load_effect(device, "data/tex.fx");

		logo_tex = engine::load_texture(device, "data/logo.png");
		logo = Image(logo_tex, tex_fx, polygon);

		CComPtr<IDirect3DCubeTexture9> env;
		d3d_err(D3DXCreateCubeTextureFromFile(device, "data/stpeters_cross2.dds", &env));
		test_fx->SetTexture("env", env);

		blurme1_tex = RenderTexture(device, backbuffer.get_desc().Width, int(backbuffer.get_desc().Height * aspect * (9 / 16.f)));
		blurme2_tex = RenderTexture(device, backbuffer.get_desc().Width, int(backbuffer.get_desc().Height * aspect * (9 / 16.f)));

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

		device->BeginScene();

		if (true)
		{
			device->SetRenderTarget(0, blurme1_tex.get_surface());
			device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0x868e91, 1.f, 0);

			// setup projection
			D3DXMATRIX proj;

			D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(80.f), 16.f / 9, 1.f, 100000.f);
			device->SetTransform(D3DTS_PROJECTION, &proj);

			float rot = time;

	//		srand(cam_seed.getIntValue());
			srand(int(beat));

			// setup camera (animate parameters)
			D3DXVECTOR3 at(0, 0.f, 10.f);
			D3DXVECTOR3 up(0.f, 0.f, 1.f);
			D3DXVECTOR3 eye(
				sin(rot * 0.25f + randf() * 1.f) * 10, // cam_rand.getFloatValue(),
				cos(rot * 0.25f + randf() * 1.f) * 10, // cam_rand.getFloatValue(),
				50.f + randf() * cam_rand.getFloatValue());

			// setup camera (view matrix)
			D3DXMATRIX view;
			D3DXMatrixLookAtLH(&view, &eye, &at, &up);
			device->SetTransform(D3DTS_VIEW, &view);

			// setup object matrix
			D3DXMATRIX world;
			D3DXMatrixIdentity(&world);
			device->SetTransform(D3DTS_WORLD, &world);

			test_fx.set_matrices(world, view, proj);
			test_fx->SetTexture("map", logo_tex);
			test_fx->SetFloat("overbright", 1.0 + 1.0 / fmod(beat, 1.0));
			test_fx->CommitChanges();

			test_fx.draw(mesh2);

			Matrix4x4 texcoord_transform;
			Matrix4x4 texel_transform;
			Matrix4x4 texture_transform = texture_matrix(blurme1_tex);

			float amt = 1.0 - pow((1.0 - fmod(beat / 2, 1.0)) * 0.1, 2.0);

			texcoord_transform.make_scale(Vector3(1,1,1));
			blur_fx->SetMatrix("texcoord_transform", &texcoord_transform);
			blur_fx->SetMatrix("texture_transform", &texture_transform);

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
		}

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
		tex_transform.make_scale(Vector3(pow(1.0 - fmod(beat / 4, 1.0), 8.0) * cos(beat * M_PI * 4) * 0.25 + 1.0, 1, 1));
		tex_fx->SetMatrix("tex_transform", &tex_transform);
		logo.draw(device);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);

		device->EndScene();
		HRESULT res = device->Present(0, 0, 0, 0);
		if (FAILED(res))
		{
			throw FatalException(std::string(DXGetErrorString9(res)) + std::string(" : ") + std::string(DXGetErrorDescription9(res)));
		}

		if (time > END_TIME) done = true;
	}
};

#endif // MEGADEMO_H
