string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;
float alpha;

// textures
texture tex1;
texture tex2;

sampler tex1_sampler = sampler_state
{
	Texture = (tex1);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

sampler tex2_sampler = sampler_state
{
	Texture = (tex2);
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
//	Out.tex = float2(Out.pos.x * 0.5 + 0.5f, -Out.pos.y * 0.5 + 0.5f);
	Out.tex = float2(Out.pos.x * 0.5 + 0.5f + (0.5f / 256), -Out.pos.y * 0.5 + 0.5f + (0.5f / 256));
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float t1 = tex2D(tex1_sampler, In.tex);
	float t2 = tex2D(tex2_sampler, In.tex);
	return t1 + (t2 - t1) * alpha;
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
