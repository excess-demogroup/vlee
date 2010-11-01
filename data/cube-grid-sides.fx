float4x4 matView : WORLDVIEW;
float4x4 matViewProjection : WORLDVIEWPROJECTION;
const float2 invMapSize = float2(1.0 / 32, 1.0 / 32);
texture cube_light_tex;
const float fog_density;

sampler light = sampler_state {
	Texture = (cube_light_tex);
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

texture n1_tex;
sampler n1 = sampler_state {
	Texture = (n1_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = MIRRORONCE;
	AddressV = MIRRORONCE;
	sRGBTexture = TRUE;
};

texture n2_tex;
sampler n2 = sampler_state {
	Texture = (n2_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = MIRRORONCE;
	sRGBTexture = TRUE;
};

texture n3_tex;
sampler n3 = sampler_state {
	Texture = (n3_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = MIRRORONCE;
	sRGBTexture = TRUE;
};

texture f_tex;
sampler f = sampler_state {
	Texture = (f_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = MIRRORONCE;
	sRGBTexture = TRUE;
};

texture ao_tex;
sampler cube_ao = sampler_state {
	Texture = (ao_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

struct VS_INPUT {
	float4 pos  : POSITION0;
	float2 uv   : TEXCOORD0;
	float3 norm : NORMAL;
};

struct VS_OUTPUT {
	float4 pos  : POSITION0;
	float2 cpos : TEXCOORD0;
	float  fog  : TEXCOORD1;
	float4 uv   : TEXCOORD2;
	float3 n    : TEXCOORD3;
	float2 uv2  : TEXCOORD4;
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT o;
	o.pos = mul(Input.pos, matViewProjection);
	o.cpos = (floor(Input.pos.xz / 16) + 0.5) * invMapSize;

	o.uv.xy = Input.uv * 2 - 1;
	o.uv.z = Input.uv.x;
	o.uv.w = 1 - Input.uv.x;
	o.uv2 = Input.uv;
	o.n = float3(Input.norm.xz * invMapSize, 0);

	float eyez = mul(Input.pos, matView).z;
	o.fog = exp(-(eyez * eyez * fog_density));

	return o;
}

float4 ps_main(VS_OUTPUT i) : COLOR0
{
	float3 c = tex2D(light, i.cpos.xy).rgb * 5;
	float att = i.fog;
	float ao = tex2D(cube_ao, i.uv2).r * 0.005;

	i.cpos.xy += i.n.xy;
	c += tex2D(n1, i.uv.xy).r * tex2D(light, i.cpos.xy).rgb;
	c += tex2D(n2, i.uv.zy).r * tex2D(light, i.cpos.xy - i.n.zx).rgb;
	c += tex2D(n3, i.uv.zy).r * tex2D(light, i.cpos.xy - i.n.zx * 2).rgb;
	c += tex2D(n2, i.uv.wy).r * tex2D(light, i.cpos.xy + i.n.zx).rgb;
	c += tex2D(n3, i.uv.wy).r * tex2D(light, i.cpos.xy + i.n.zx * 2).rgb;

	i.cpos.xy += i.n.xy;
	c += tex2D(f, i.uv.zy).r * tex2D(light, i.cpos.xy - i.n.zx).rgb;
	c += tex2D(f, i.uv.wy).r * tex2D(light, i.cpos.xy + i.n.zx).rgb;

	return float4((ao + c * 2) * i.fog, 1.0);
}

technique cube_sides {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}

technique cube_sides2 {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
