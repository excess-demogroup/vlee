string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;

// textures
texture tex;

sampler samp = sampler_state
{
	Texture = (tex);
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

struct VS_OUTPUT
{
	float4 pos  : POSITION;
	float3 tex  : TEXCOORD1;
};

VS_OUTPUT vertex(
	float3 ipos  : POSITION,
	float3 itex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos  = mul(float4(ipos,  1), WorldViewProjection);
	Out.tex = itex;
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	return tex2D(samp, In.tex);
}

technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
