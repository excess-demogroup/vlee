float4x4 matView : WORLDVIEW;
float4x4 matViewProjection : WORLDVIEWPROJECTION;
const int2 mapSize = int2(32, 32);

texture cube_light_tex;
sampler light = sampler_state {
	Texture = (cube_light_tex);
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

texture ao_tex;
sampler floor_ao = sampler_state {
	Texture = (ao_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = MIRRORONCE;
	AddressV = MIRRORONCE;
	sRGBTexture = FALSE;
};

texture radiosity_tex;
sampler radiosity = sampler_state {
	Texture = (radiosity_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = MIRRORONCE;
	AddressV = MIRRORONCE;
	sRGBTexture = TRUE;
};

struct VS_INPUT {
	float4 pos : POSITION0;
	float2 uv  : TEXCOORD0;
};

struct VS_OUTPUT {
	float4 pos  : POSITION0;
	float2 cpos : TEXCOORD0;
	float  fog  : TEXCOORD1;
	float2 uv   : TEXCOORD2;
};

VS_OUTPUT vs_main(VS_INPUT i)
{
	VS_OUTPUT o;
	o.pos = mul(i.pos, matViewProjection);
	float3 p = i.pos.xyz / 16;
	o.cpos = floor(p.xz);

	float eyez = mul(i.pos, matView).z;
	o.fog = exp(-(eyez * eyez * 0.004));

	o.uv = (i.uv - 0.5);
	o.cpos /= mapSize;
	return o;
}

float4 ps_main(VS_OUTPUT i) : COLOR0
{
	float att = i.fog;
	float ao = tex2D(floor_ao, i.uv * 2).r * 0.005;

	float3 c = 0;
	for (int y = -1; y < 2; ++y) {
		float2 pos = i.uv / 1.5 + float2(1.0 / 1.5, y / 1.5);
		float2 cpos = i.cpos - float2(1.0 / 64, -y / 64.0);
		for (int x = -1; x < 2; ++x) {
			c += tex2D(light, cpos).rgb * tex2D(radiosity, pos).r;
			pos.x -= 1.0 / 1.5;
			cpos.x += 1.0 / 64;
		}
	}
	return float4(ao + c * 2, 1.0);
}

technique cube_floor {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
