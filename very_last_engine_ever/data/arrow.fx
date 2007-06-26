float alpha = 1.f;
texture tex;
float time;

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

VS_OUTPUT vertex(float4 ipos : POSITION, float2 tex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos = ipos;
	Out.tex = tex;
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 texcol = tex2D(tex_sampler, In.tex);
//	texcol.r = sin(texcol.r + time);
	texcol.r = lerp(0.0, frac(In.tex.x + time), texcol.r);
	texcol.g = lerp(0.0, frac(In.tex.x + time - 0.5), texcol.g);
	return texcol * alpha;
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
