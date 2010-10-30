string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

float2 dir = {0.0, 0.0};
#define SAMPLE_COUNT 8
const float alpha[SAMPLE_COUNT] = { 0.03f, 0.075f, 0.15f, 0.3f, 0.3f, 0.15f, 0.075f, 0.03f };

// textures
texture blur_tex;

sampler blur_sampler = sampler_state {
	Texture = (blur_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = TRUE;
};

struct VS_OUTPUT {
	float4 pos  : POSITION;
	float2 tex  : TEXCOORD1;
};

VS_OUTPUT vertex(float4 ipos : POSITION)
{
	VS_OUTPUT Out;
	Out.pos = ipos;
	Out.tex = float2(
		 Out.pos.x * 0.5 + 0.5f + (0.5f / 256) - (SAMPLE_COUNT / 2 - 0.5) * dir.x,
		-Out.pos.y * 0.5 + 0.5f + (0.5f / 256) - (SAMPLE_COUNT / 2 - 0.5) * dir.y);
//	Out.tex = float2(Out.pos.x * 0.5 + 0.5f - (0.5f / 256), -Out.pos.y * 0.5 + 0.5f - (0.5f / 256));
	return Out;
}

float sub;

float4 pixel(VS_OUTPUT In) : COLOR
{
//	float4 color = tex2D(blur_sampler, In.tex);
	float4 color = 0;
	float2 tex = In.tex;
	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		color += tex2D(blur_sampler, tex) * alpha[i];
		tex += dir;
	}
	return max(color - sub, 0.0);
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
