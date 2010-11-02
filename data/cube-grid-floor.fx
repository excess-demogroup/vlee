float4x4 matView : WORLDVIEW;
float4x4 matViewProjection : WORLDVIEWPROJECTION;
const int2 mapSize = int2(32, 32);
const float2 invMapSize = float2(1.0 / 32, 1.0 / 32);
const float fog_density;

texture cube_light_tex;
sampler light = sampler_state {
	Texture = (cube_light_tex);
	AddressU = WRAP;
	AddressV = WRAP;
	MagFilter = POINT;
	MinFilter = POINT;
	MipFilter = NONE;
	sRGBTexture = FALSE;
};

texture ao_tex;
sampler floor_ao = sampler_state {
	Texture = (ao_tex);
	AddressU = MIRROR;
	AddressV = MIRROR;
	MagFilter = LINEAR;
	MinFilter = LINEAR;
	MipFilter = LINEAR;
	sRGBTexture = FALSE;
};

texture l_tex;
sampler r = sampler_state {
	Texture = (l_tex);
	AddressU = MIRRORONCE;
	AddressV = MIRRORONCE;
	MagFilter = LINEAR;
	MinFilter = LINEAR;
	MipFilter = NONE;
	sRGBTexture = TRUE;
};

struct VS_INPUT {
	float4 pos : POSITION0;
	float2 uv  : TEXCOORD0;
};

struct VS_OUTPUT {
	float4 pos  : POSITION0;
	float  fog  : TEXCOORD1;
	float2 uv   : TEXCOORD2;
};

VS_OUTPUT vs_main(VS_INPUT i)
{
	VS_OUTPUT o;
	o.pos = mul(i.pos, matViewProjection);
	float eyez = mul(i.pos, matView).z;
	o.fog = exp(-(eyez * eyez * fog_density));
	o.uv = float2(i.uv.x, -i.uv.y) * mapSize;
	return o;
}


float4 ps_main(VS_OUTPUT i) : COLOR0
{
	float ao = tex2D(floor_ao, i.uv * 2 - 1).r * 0.005;

	float3 c = 0;
	for (int y = -1; y < 2; ++y) {
		float2 pos = (frac(i.uv) * 2 - 1 + float2(-2, y * 2)) / 3;
		float2 cpos = (floor(i.uv) + float2(1, -y)) * invMapSize;
		for (int x = -1; x < 2; ++x) {
			c += tex2D(light, cpos).rgb * tex2D(r, pos).r;
			pos.x += 2.0 / 3;
			cpos.x -= invMapSize.x;
		}
	}
	return float4((ao + c * 2) * i.fog, 1.0);
}

technique cube_floor {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
