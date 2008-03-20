float alpha = 1.f;
float4x4 transform;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorld : WORLD;
float4x4 matView : VIEW;

texture2D tex;
sampler2D tex_sampler = sampler_state
{
	Texture = (tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = WRAP;
	AddressV = WRAP;
};

struct VS_OUTPUT
{
	float4 pos  : POSITION;
	float2 tex  : TEXCOORD0;
};

VS_OUTPUT vertex(float3 ipos : POSITION, float2 tex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos = mul(float4(ipos,  1.0), matWorldViewProjection);
	Out.tex = tex * float2(1, 2);
	return Out;
}

float scroll = 0.0;

float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 color = tex2D(tex_sampler, In.tex);
	
	float scrolly = floor(In.tex.y) + scroll * 2;
	clip(-scrolly);
	return color;
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
