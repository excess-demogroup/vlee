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
float4x4 WorldView : WORLDVIEW;

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
};

VS_OUTPUT vertex(VS_INPUT In)
{
	VS_OUTPUT Out;
	In.pos += left * In.tex.x * In.size;
	In.pos += up   * In.tex.y * In.size;
	
	Out.pos   = mul(float4(In.pos,  1), WorldViewProjection);
	
	Out.tex = float2(In.tex.x * 0.5, -In.tex.y * 0.5);
	Out.tex += float2(0.5f, 0.5f);
	
	return Out;
}

struct PS_OUTPUT
{
	float4 col : COLOR;
};

PS_OUTPUT pixel(VS_OUTPUT In)
{
	PS_OUTPUT output;
	output.col = tex2D(tex_samp, In.tex);
	return output;
}

technique particle_yes_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
