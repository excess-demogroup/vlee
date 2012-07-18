const float4 gauss[8];
const int size;

texture blur_tex;
sampler tex = sampler_state {
	Texture = (blur_tex);
	MipFilter = POINT;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

struct VS_OUTPUT {
	float4 pos  : POSITION;
	float2 tex  : TEXCOORD0;
};

VS_OUTPUT vertex(float4 pos : POSITION, float2 tex : TEXCOORD0)
{
	VS_OUTPUT o;
	o.pos = pos;
	o.tex = tex;
	return o;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 c = tex2D(tex, In.tex) * gauss[0].z;
	for (int i = 1; i < size; i++) {
		c += tex2D(tex, In.tex + gauss[i].xy) * gauss[i].z;
		c += tex2D(tex, In.tex - gauss[i].xy) * gauss[i].z;
	}
	return c;
}

technique blur {
	pass P0 {
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}

float3 rgbe_to_rgb(float4 rgbe)
{
	return rgbe.rgb * exp2(rgbe.a * 255 - 128);
}

float4 pixel2(VS_OUTPUT In) : COLOR
{
	return float4(rgbe_to_rgb(tex2D(tex, In.tex)), 1.0);
}

technique hack {
	pass P0 {
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel2();
	}
}
