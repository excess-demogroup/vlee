float flash = 0;
float fade  = 1;
float blend = 0;

texture bloom;
sampler bloom_sampler = sampler_state {
	Texture = (bloom);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

texture tex;
sampler tex_sampler = sampler_state {
	Texture = (tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

struct VS_OUTPUT {
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

float luminance(float3 color)
{
	return color.r * 0.299 +
	       color.g * 0.587 +
	       color.b * 0.114;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 color = tex2D(tex_sampler, In.tex);
	color += pow(tex2D(bloom_sampler, In.tex) * 0.75, 1.5);
	return color * fade + flash;
}

technique color_map {
	pass P0 {
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
