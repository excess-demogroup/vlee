string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

float2 dir = {0, 0};

texture tex;
sampler tex_sampler = sampler_state
{
	Texture = (tex);
	MinFilter = POINT;
	MagFilter = POINT;
	MipFilter = NONE;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

struct VS_OUTPUT
{
	float4 pos  : POSITION;
	float2 tex  : TEXCOORD1;
};

VS_OUTPUT vertex(float4 ipos : POSITION)
{
	VS_OUTPUT Out;
	Out.pos = ipos;
//	Out.tex = float2(Out.pos.x * 0.5 + 0.5f, -Out.pos.y * 0.5 + 0.5f);
	Out.tex = float2(Out.pos.x * 0.5 + 0.5 + (0.5 / 800), -Out.pos.y * 0.5 + 0.5f + (0.5 / 600));
	Out.tex -= dir * 3.5;
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 color = 0;
	float2 tex = In.tex;
	for (int i = 0; i < 8; i++)
	{
		color += tex2D(tex_sampler, tex);
		tex += dir;
	}
	return color *= 1.0 / 8;

	float4 icol = tex2D(tex_sampler, In.tex);
	float4 ocol = icol;
	return ocol;
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
