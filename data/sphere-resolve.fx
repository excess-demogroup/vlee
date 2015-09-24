texture tex;
sampler samp = sampler_state {
	Texture = (tex);
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
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
	float4 col = tex2D(samp, In.tex);
	float ao = 1 - col.a;
	return float4(col.rgb * ao, 1);
}

technique blur {
	pass P0 {
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 pixel();
	}
}
