string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

float alpha = 1.f;
float xoffs = 0.f;
float yoffs = 0.f;

float xzoom = 1.f;
float yzoom = 1.f;
float4x4 tex_transform;

float3 up;
float3 left;
float3 fog_color = float3(1, 1, 1);


float4x4 WorldViewProjection : WORLDVIEWPROJECTION;

// textures
texture tex;

sampler tex_samp = sampler_state
{
	Texture = (tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

struct VS_INPUT
{
	float3 pos  : POSITION;
	float  size : TEXCOORD1;
	float2 tex  : TEXCOORD2;
};

struct VS_OUTPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
	float  z : TEXCOORD1;
};

VS_OUTPUT vertex(VS_INPUT In)
{
	VS_OUTPUT Out;
/*	In.pos += dot(left, In.tex) * In.size;
	In.pos += dot(up,   In.tex) * In.size; */
	In.pos += left * In.tex.x * In.size;
	In.pos += up   * In.tex.y * In.size;
	Out.pos  = mul(float4(In.pos,  1), WorldViewProjection);
	
//	Out.pos.x += In.tex.x * In.size;
//	Out.pos.y += In.tex.y * In.size;
	Out.z = 1 - clamp(1.0f / (Out.pos.z / 10), 0.0, 1.0);
	
	Out.tex = float2(In.tex.x * 0.5, -In.tex.y * 0.5);
	Out.tex += float2(0.5f, 0.5f);
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float fog_factor = In.z;
	float4 col = tex2D(tex_samp, In.tex);
	return float4(lerp(col.xyz, fog_color, fog_factor), col.a);
//	return tex2D(tex_samp, In.tex);
//	return float4(In.tex, 0, 0);
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
