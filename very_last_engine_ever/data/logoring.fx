// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;

float alpha = 1.0;

texture2D tex;
sampler2D tex_samp = sampler_state
{
	Texture = (tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
};

struct VS_OUTPUT 
{
	float4 pos        : POSITION0;
	float2 tex        : TEXCOORD0;
};

VS_OUTPUT vertex(
	float3 ipos  : POSITION,
	float3 inorm : NORMAL,
	float3 itex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos = mul(float4(ipos,  1), WorldViewProjection);
	Out.tex = itex.yx;
//	Out.tex = ipos.xz;
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	return float4(2,0,2, 1) * tex2D(tex_samp, In.tex) * alpha;
//	return In.tex.xxxx - 1;
}

technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
