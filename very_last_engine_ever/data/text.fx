float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorld : WORLD;
float4x4 matView : VIEW;

uniform float4 vViewPosition;

struct VS_INPUT 
{
	float3 Pos:      POSITION;
	float2 texcoord : TEXCOORD0;
};

struct VS_OUTPUT 
{
	float4 Pos:     POSITION;
	float2 texcoord0 : TEXCOORD0;
	float2 texcoord1 : TEXCOORD1;
};

VS_OUTPUT vs_main( VS_INPUT In )
{
	VS_OUTPUT Out;

	Out.Pos           = mul(float4(In.Pos,  1.0), matWorldViewProjection);
	Out.texcoord0      = In.texcoord;
	Out.texcoord1      = (In.texcoord - 0.5) * 1.2 + 0.5;

	return Out;
}

#define PS_INPUT VS_OUTPUT

texture excess;
sampler excess_sampler = sampler_state
{
	Texture = (excess);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

texture demotitle;
sampler demotitle_sampler = sampler_state
{
	Texture = (demotitle);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

float4 ps_main( PS_INPUT In ) : COLOR0
{
	return
		tex2D(excess_sampler, In.texcoord0) +
		tex2D(demotitle_sampler, In.texcoord1);
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
