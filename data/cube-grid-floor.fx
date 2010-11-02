float4x4 matView : WORLDVIEW;
float4x4 matViewProjection : WORLDVIEWPROJECTION;
static const int2 mapSize = int2(32, 32);
static const float2 invMapSize = 1.0 / mapSize;
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

static const float2 possy[3] = {
	float2(-2, -2) / 3,
	float2(-2,  0) / 3,
	float2(-2,  2) / 3
};

static const float2 cpossy[3] = {
	float2(1, -1) * invMapSize,
	float2(1,  0) * invMapSize,
	float2(1,  1) * invMapSize
};

float4 ps_main(VS_OUTPUT i) : COLOR0
{
	float ao = tex2D(floor_ao, i.uv * 2 - 1).r * 0.005;

	float3 c = 0;
	for (int y = 0; y < 3; ++y) {
		float2 pos = (frac(i.uv) * 2 - 1) / 3 + possy[y];
		float2 cpos = floor(i.uv) * invMapSize + cpossy[y];
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

float4 rgb_to_ergb(float3 rgb)
{
	float e = ceil(log2(max(max(rgb.r, rgb.g), rgb.b)));
	return float4(rgb * exp2(-e), (e + 128) / 255);
}

float4 ps_main_rgbe(VS_OUTPUT i) : COLOR0
{
	return rgb_to_ergb(ps_main(i).rgb);
}

technique rgbe {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main_rgbe();
	}
}
