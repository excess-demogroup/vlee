float alpha = 1.0;
const float3 up;
const float3 left;
const float4x4 WorldViewProjection : WORLDVIEWPROJECTION;

// textures
texture tex;
sampler tex_samp = sampler_state {
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
	float  size : TEXCOORD0;
	float2 uv   : TEXCOORD1;
};

struct VS_OUTPUT
{
	float4 pos : POSITION;
	float2 uv  : TEXCOORD0;
};

VS_OUTPUT vertex(VS_INPUT In)
{
	VS_OUTPUT Out;
	In.pos += left * In.uv.x * In.size;
	In.pos += up   * In.uv.y * In.size;
	Out.pos = mul(float4(In.pos,  1), WorldViewProjection);
	Out.uv = (In.uv + 1) / 2;
	return Out;
}

float4 pixel(VS_OUTPUT In)  : COLOR
{
	return tex2D(tex_samp, In.uv) * alpha;
}

technique fx {
	pass P0 {
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
