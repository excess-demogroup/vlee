string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

#define ITERATIONS 4

float4x4 texcoord_transform;
float2x2 texture_transform;
float3x3 texel_transform;
float    brightness  = 1.0 / ITERATIONS;
float2 texcoord_translate = float2(0.5, 0.5);

texture tex;
sampler tex_sampler = sampler_state
{
	Texture = (tex);
	MinFilter = LINEAR;
	MagFilter = LINEAR;
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
	Out.tex = mul(ipos, texcoord_transform);
//	Out.tex.xy = float2(Out.tex.x * 0.5 + 0.5f, -Out.tex.y * 0.5 + 0.5f);
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 color = 0.0;	
	float2 tex = float2(In.tex);
	for (int i = 0; i < ITERATIONS; i++)
	{
		color += tex2D(tex_sampler, float2(tex.x * 0.5, -tex.y * 0.5) + texcoord_translate);
 		tex = mul(float3(tex, 1.0), texel_transform);
	}
	return color * brightness;
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
