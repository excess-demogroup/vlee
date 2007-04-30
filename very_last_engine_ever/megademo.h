#ifndef MEGADEMO_H
#define MEGADEMO_H

#define END_TIME (60 * 3 + 30) /* 3:30 */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#include "renderer/texture.h"
#include "renderer/rendertexture.h"
#include "renderer/device.h"
#include "engine/demo.h"
#include "engine/image.h"
#include "engine/anim.h"
#include "engine/particlestreamer.h"
#include "engine/particlecloud.h"
#include "engine/spectrumdata.h"
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
	Texture load_texture(renderer::Device &device, ::std::string filename)
	{
		Texture tex;

		HRESULT hr = D3DXCreateTextureFromFile(device, filename.c_str(), &tex);
		if (FAILED(hr)) throw core::FatalException(::std::string("failed to load mesh \"") + filename + ::std::string("\"\n\n") + core::d3d_get_error(hr));

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

bool blink_inout(float time, float startblink, float endblink, float blinklen)
{
	return ((time > (startblink + blinklen) && time < (endblink - blinklen)) || ((time > startblink && time < endblink) && (int(time) & 1)));
}

Matrix4x4 texture_matrix(const Texture &tex)
{
	Matrix4x4 mat, trans1, trans2, scale;

	//	D3DXMatrixIdentity(&trans2);
//	trans1._31 = 0.5;
//	trans1._32 = 0.5;

	D3DXMatrixIdentity(&trans2);
	trans2._31 = -10.5f / tex.get_surface().get_desc().Width;
	trans2._32 = -10.5f / tex.get_surface().get_desc().Height;

	D3DXMatrixScaling(&scale, 0.5, 0.5, 1.0);
	return trans2;
//	mat = trans2 * scale;

//	mat = trans1 * scale;
	return mat;
}

Matrix4x4 radialblur_matrix(const Texture &tex, const Vector2 &center, const float amt = 1.01)
{
	Matrix4x4 mat;
	mat.make_scaling(Vector3(amt, amt, 1.0));
#if 0
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
	Effect tunelle_fx;

	Mesh    kuber_mesh;
	Texture kuber_tex;
	Effect  kuber_fx;

	Effect test_fx;
	Effect blur_fx;
	Effect tex_fx;
	Effect particle_fx;

	Texture logo_tex;
	Image logo;

	Texture analog_tex;
	Image analog;

	Texture ramme_tex;
	Image ramme;

	Texture end_tex;
	Image end;

	Texture end2_tex;
	Image end2;

	Texture greets_tex;
	Image greets;

	Texture fucks_tex;
	Image fucks;

	RenderTexture blurme1_tex;
	Image blurme1;

	RenderTexture blurme2_tex;
	Image blurme2;

	Texture bartikkel_tex;
	CComPtr<IDirect3DCubeTexture9> env;

//	engine::TextureProxy texloader;

/*	engine::bass::Stream music;
	engine::core::Texture tex;
	engine::scenegraph::Scene scene; */

	Sync &sync;
	SyncTrack &fade, &flash, &part;
	SyncTrack &xrot, &yrot, &zrot;
	SyncTrack &zrot2, &scrolly;
	SyncTrack &cam_seed, &cam_rand;
	SyncTrack &cam_x, &cam_y, &cam_z;
	SyncTrack &at_x, &at_y, &at_z;
	SyncTrack &cam_fov;
	SyncTrack &blur_amt;
	SyncTrack &clear_color_param;
	SyncTrack &morph;
	SyncTrack &vid_track;
	SyncTrack &vid_flip;

	std::vector<engine::Anim> vids;

	engine::SpectrumData spec;

	engine::ParticleStreamer streamer;
	engine::ParticleCloud<float> cloud;

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
		zrot2( sync.getTrack("z2",      "rotation", 5, true)),
		scrolly( sync.getTrack("scrolly",      "rotation", 5, true)),

		vid_track( sync.getTrack("track",      "vid", 5, true)),
		vid_flip(  sync.getTrack("flip",      "vid", 5, true)),

		cam_x( sync.getTrack("x",       "cam", 5, true)),
		cam_y( sync.getTrack("y",       "cam", 5, true)),
		cam_z( sync.getTrack("z",       "cam", 5, true)),
		cam_fov( sync.getTrack("fov",   "cam", 5, true)),
		cam_seed( sync.getTrack("seed", "cam", 5, true)),
		cam_rand( sync.getTrack("rand", "cam", 5, true)),
		at_x( sync.getTrack("x",       "at", 5, true)),
		at_y( sync.getTrack("y",       "at", 5, true)),
		at_z( sync.getTrack("z",       "at", 5, true)),
		clear_color_param( sync.getTrack("clear",       "global", 5, true)),
		morph( sync.getTrack("morph",       "global", 5, true))
	{
		streamer = engine::ParticleStreamer(device);
		
/*		tex   = texloader.get("test.jpg");
		scene = sceneloader.get("test.scene");
		music = musicloader.get(); */
		
		d3d_err(D3DXCreatePolygon(device, 3.f, 4, &polygon, 0));
//		mesh2 = engine::load_mesh(device, "data/test.x");

		tunelle_mesh = engine::load_mesh(device, "data/tunelle.x");
		tunelle_tex  = engine::load_texture(device, "data/tunelle.dds");
		tunelle_fx  = engine::load_effect(device, "data/tunelle.fx");

		kuber_mesh = engine::load_mesh(device,    "data/kuber.x");
		kuber_tex  = engine::load_texture(device, "data/kuber.dds");
		kuber_fx  = engine::load_effect(device,   "data/kuber.fx");

		test_fx     = engine::load_effect(device, "data/test.fx");
		blur_fx     = engine::load_effect(device, "data/blur.fx");
		tex_fx      = engine::load_effect(device, "data/tex.fx");
		particle_fx = engine::load_effect(device, "data/particle.fx");
		
		logo_tex = engine::load_texture(device, "data/logo.png");
		logo = Image(logo_tex, tex_fx, polygon);

		analog_tex = engine::load_texture(device, "data/analog.dds");
		analog = Image(analog_tex, tex_fx, polygon);

		ramme_tex = engine::load_texture(device, "data/ramme.png");
		ramme = Image(ramme_tex, tex_fx, polygon);
		
		end_tex = engine::load_texture(device, "data/end.dds");
		end = Image(end_tex, tex_fx, polygon);

		end2_tex = engine::load_texture(device, "data/end2.dds");
		end2 = Image(end2_tex, tex_fx, polygon);

		greets_tex = engine::load_texture(device, "data/greets.dds");
		greets = Image(greets_tex, tex_fx, polygon);
/*
		fucks_tex = engine::load_texture(device, "data/fucks.dds");
		fucks = Image(fucks_tex, tex_fx, polygon);
*/
		bartikkel_tex = engine::load_texture(device, "data/bartikkel.dds");
		
		d3d_err(D3DXCreateCubeTextureFromFile(device, "data/stpeters_cross2.dds", &env));
		test_fx->SetTexture("env", env);

		log::printf("aspect correction: %f\n", aspect * (16.f / 9));
		blurme1_tex = RenderTexture(device, backbuffer.get_desc().Width, int(backbuffer.get_desc().Width / (16.f / 9.f)));
		blurme2_tex = RenderTexture(device, backbuffer.get_desc().Width, int(backbuffer.get_desc().Width / (16.f / 9.f)));
#if 1
		for (int i = 0; i < 1024 * 8; ++i)
		{
#if 1
			float x = (randf() - 0.5f) * 200;
			float z = (randf() - 0.5f) * 200;
			float y = (randf() - 0.5f) * 200;
			float s = (randf() + 0.5f) * 3.f;
#elif 0
			float p = i;
			float rad = 50 + sin(p * 0.0001f) * 10;
			float x = (sin(p * 0.01)) * rad;
			float z = (cos(p * 0.01)) * rad;
			float y = (sin(p * 0.032)) * 10;
			float s = 3.f + sin(p * 0.189) * 2;
			s *= 0.5;
#endif
			cloud.particles.insert(cloud.particles.end(), engine::Particle<float>(Vector3(x, y, z), s));
		}
#else
		for (int j = 0; j < 1; ++j)
		{
			float x = (randf() - 0.5f);
			float z = (randf() - 0.5f);
			float y = (randf() - 0.5f);
			Vector3 pos(x, y, z);
			pos.normalize();

			Vector3 dir = pos;

			const int max = 4096;
			for (int i = 0; i < max; ++i)
			{
//				cloud.particles.insert(cloud.particles.end(), engine::Particle(pos, 1.0 / (1 + (i * 0.05f)) ));
//				float s = min(float(i) / max, float(max - i) / max);
				float s = float(i) / max;

				pos = Vector3(cos(s * M_PI * 10), sin(s * M_PI * 8), cos(1 - s * M_PI * 9));
				pos.x += cos(pos.y * 2);
				pos.y += cos(pos.z * 2);
				pos.z += cos(pos.x * 2);

				pos *= 3;
//				pos *= sin(s);

				s = 1 - cos(s * M_PI * 2);
				s = (1 + cos(s * M_PI * 10) * 0.75) * s;
				s *= 0.35;

				cloud.particles.insert(cloud.particles.end(), engine::Particle(pos, s));
				pos += dir * 0.05;
			}
		}
#endif
		vids.push_back(engine::load_anim(device, "data/vid1"));
		vids.push_back(engine::load_anim(device, "data/vid2"));
		vids.push_back(engine::load_anim(device, "data/vid3"));
		vids.push_back(engine::load_anim(device, "data/vid4"));
		spec = engine::load_spectrum_data("data/test.fft");
	}
	
	void start()
	{
/*		music.play(); */
	}
	
	void draw(float time)
	{
		time -= 10.f / (1000000);
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

		float spec1_val = spectrum[0];

		D3DXCOLOR clear_color;
		clear_color.r = clear_color_param.getFloatValue() * (1.0f / 256);
		clear_color.g = clear_color_param.getFloatValue() * (1.0f / 256);
		clear_color.b = clear_color_param.getFloatValue() * (1.0f / 256);

		clear_color.r = 0;
		clear_color.g = 0;
		clear_color.b = 0;
		clear_color.r = spec.getValue(time) / 2;
		static float last_blink = 0.f;
		float blink = last_blink + (spectrum[0] - last_blink) * 0.1f;
		last_blink = blink;

		blink *= 0.05f;
#if 0
		clear_color.r += blink;
		clear_color.g += blink;
		clear_color.b += blink;
#endif
		particle_fx->SetFloatArray("fog_color", clear_color, 3);



		if (true)
		{
			device->SetRenderTarget(0, blurme1_tex.get_surface());
			device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, clear_color, 1.f, 0);

			// setup projection
			D3DXMATRIX proj;

			D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(cam_fov.getFloatValue()), 16.f / 9, 0.01f, 1000.f);
			D3DXMatrixPerspectiveFovLH(&proj, D3DXToRadian(90.0f), float(blurme1_tex.get_surface().get_desc().Width) / blurme1_tex.get_surface().get_desc().Height, 0.01f, 1000.f);
			device->SetTransform(D3DTS_PROJECTION, &proj);

			float rot = time * 0.4f;

	//		srand(cam_seed.getIntValue());
			srand(int(beat));

			// setup camera (animate parameters)

			srand(0);
			Vector3 at(0, 0.f, 0.f);
			at = Vector3(at_x.getFloatValue(), at_y.getFloatValue(), at_z.getFloatValue());
			D3DXVECTOR3 up(0.f, 1.f, 0.f);
			Vector3 eye = Vector3
			(
				cam_x.getFloatValue(), // cam_rand.getFloatValue(),
				cam_y.getFloatValue(), // cam_rand.getFloatValue(),
				cam_z.getFloatValue()
			); // cam_rand.getFloatValue(),
/*
			eye = Vector3(
				sin(time) * 4,
				0.f,
				cos(time) * 4
			);
*/
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
			if (beat < 256 - 64)
			{

				if (beat > (64 + 8))
				{
					tunelle_fx->SetFloatArray("fog_color", clear_color, 3);
					for (int i = 0; i < 24; ++i)
					{
						Vector3 scale(0.1, 0.1, 0.1);
						Vector3 translation(0,0,i * 25);
						D3DXQUATERNION rotation;
						D3DXQuaternionRotationYawPitchRoll(&rotation, 0, float(M_PI / 2), 0);
						D3DXQUATERNION rotation2;
						D3DXQuaternionRotationYawPitchRoll(&rotation2, 0, 0, zrot.getFloatValue());

						rotation *= rotation2;

						Matrix4x4 world;
						D3DXMatrixTransformation(&world, NULL, NULL, &scale, NULL, &rotation, &translation);

						tunelle_fx.set_matrices(world, view, proj);
						tunelle_fx->SetTexture("map", tunelle_tex);
						tunelle_fx->SetFloat("overbright", 1.0);
						tunelle_fx->CommitChanges();
						device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
						tunelle_fx.draw(tunelle_mesh);
					}
				}

				if (beat > (64 + 64))
				{
					Vector3 scale(0.05, 0.05, 0.05);
					Vector3 translation(0, 0, scrolly.getFloatValue());
					D3DXQUATERNION rotation;
					D3DXQuaternionRotationYawPitchRoll(&rotation, 0, 0, zrot2.getFloatValue());

					Matrix4x4 world;
					D3DXMatrixTransformation(&world, NULL, NULL, &scale, NULL, &rotation, &translation);

					kuber_fx->SetFloatArray("fog_color", clear_color, 3);
					kuber_fx->SetFloat("morph", morph.getFloatValue() * (1.f / 256));
					kuber_fx.set_matrices(world, view, proj);
					kuber_fx->SetTexture("map", kuber_tex);
					kuber_fx->SetTexture("env", env);
					kuber_fx->CommitChanges();
					kuber_fx.draw(kuber_mesh);
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
				
				std::list<engine::Particle<float> >::const_iterator iter = cloud.particles.begin();
				bool iter_done = false;
				
				while (!iter_done)
				{
					streamer.begin();
					for (int i = 0; i < 1024; ++i)
					{
						streamer.add(
							Vector3(iter->pos.x,
									iter->pos.y,
									iter->pos.z
							),
							iter->data // + (1 - fmod(beat, 1.0)) * 2
						);
						++iter;

						if (cloud.particles.end() == iter)
						{
							iter_done = true;
							break;
						}
					}
					streamer.end();
					particle_fx.draw(streamer);
				}

				device->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
				device->SetRenderState(D3DRS_ZWRITEENABLE, true);
			}
			else
			{
				int vid = vid_track.getIntValue() % vids.size();

				float tbeat = float(beat);
				if (vid == 1) beat *= 2;

				float x_scale = 1.0f;
				if (vid_flip.getIntValue() & 1) x_scale = -1.0f;

				Matrix4x4 tex_transform;
				tex_transform.make_scaling(Vector3(x_scale, 1, 1));
				tex_fx->SetMatrix("tex_transform", &tex_transform);

				blit(device, vids[vid].getFramePingPong(float(beat)), tex_fx, polygon);

				if (beat > (512 + 128 + 32 - 8))
				{
					tex_fx->SetFloat("alpha", 1.0);
					end2.draw(device);
				}

			}

			Matrix4x4 texcoord_transform;
			Matrix4x4 texel_transform;
			Matrix4x4 texture_transform = texture_matrix(blurme1_tex);

//			float amt = 1.0 - pow((1.0 - fmod(beat / 2, 1.0)) * 0.1, 2.0);
			float amt = 1.f - pow((blur_amt.getFloatValue()) * 0.1f, 2.f);

			Vector3 texcoord_translate(
				0.5 + (0.5 / blurme1_tex.get_surface().get_desc().Width),
				0.5 + (0.5 / blurme1_tex.get_surface().get_desc().Height),
				0.0);
			blur_fx->SetFloatArray("texcoord_translate", texcoord_translate, 2);
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

		device.set_render_target(backbuffer);
		D3DVIEWPORT9 viewport;
		device->GetViewport(&viewport);
		const float correction = (aspect * (9 / 16.f));
		int old_height = viewport.Height;
		int new_height = int(viewport.Height * correction);
		viewport.Y      = (old_height - new_height) / 2;
		viewport.Height = new_height;

		// draw it all
		device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DXCOLOR(0.f, 0.f, 0.f, 0.f), 1.f, 0);
		device->SetViewport(&viewport);
		blit(device, blurme2_tex, blur_fx, polygon);

		if (time > END_TIME) done = true;
	}
};

#endif // MEGADEMO_H
