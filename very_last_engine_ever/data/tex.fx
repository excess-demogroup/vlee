string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

float alpha = 1.f;
float xoffs = 0.f;
float yoffs = 0.f;

float xzoom = 1.f;
float yzoom = 1.f;

// textures
texture tex;

sampler tex_sampler = sampler_state
{
	Texture = (tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
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
//	Out.tex = float2(Out.pos.x * 0.5 + 0.5f, -Out.pos.y * 0.5 * sin(1 + yoffs * 5) + 0.5f);
//	Out.tex = float2(Out.pos.x * 0.5 + 0.5f + (0.5f / 256) + xoffs, -Out.pos.y * 0.5 + 0.5f + (0.5f / 256) * cos(1 + xoffs * 5) + yoffs);
	Out.tex = float2(Out.pos.x * 0.5 + 0.5f + (0.5f / 256) + xoffs, -Out.pos.y * 0.5 + 0.5f + (0.5f / 256) + yoffs);
	Out.tex -= float2(0.5f, 0.5f);
	Out.tex *= float2(xzoom, yzoom);
	Out.tex += float2(0.5f, 0.5f);
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	return tex2D(tex_sampler, In.tex) * alpha;
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
