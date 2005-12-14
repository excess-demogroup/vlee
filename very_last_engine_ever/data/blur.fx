string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

float2 dir = {0.0, 0.0};
float alpha[8];

// textures
texture blur_tex;

sampler blur_sampler = sampler_state
{
	Texture = (blur_tex);
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
	
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
	Out.tex = float2(Out.pos.x * 0.5 + 0.5f + (0.5f / 256) - 4 * dir.x, -Out.pos.y * 0.5 + 0.5f + (0.5f / 256) - 4 * dir.y);
//	Out.tex = float2(Out.pos.x * 0.5 + 0.5f - (0.5f / 256), -Out.pos.y * 0.5 + 0.5f - (0.5f / 256));
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
//	float4 color = tex2D(blur_sampler, In.tex1);
//	color += tex2D(blur_sampler, In.tex2);

	float4 color = 0;
	float2 tex = In.tex;
	for (int i = 0; i < 8; i++)
	{
		color += tex2D(blur_sampler, tex) * alpha[i];
		tex += dir;
	}
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
