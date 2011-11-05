const float zoom, alpha;

texture cloud1_tex;
sampler cloud1_samp = sampler_state {
	Texture = (cloud1_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

struct VS_OUTPUT {
	float4 pos : POSITION;
	float2 uv  : TEXCOORD0;
};

VS_OUTPUT vertex(float4 ipos : POSITION, float2 uv : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos = ipos;
	Out.uv = uv;
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float f = frac(zoom);
	float2 pos = 0.5 + (In.uv - 0.5) * exp2(f) * 16;
	float3 col = tex2D(cloud1_samp, pos);
	for (int i = 0; i < 8; ++i) {
		pos = 0.5 + (pos - 0.5) * 0.5;

		float layah = floor(zoom) + i;
		float rot = layah * 1.5 - 0.75;
		float2 r = float2(sin(rot), cos(rot));
		float2 rpos = float2(
			0.5 + dot(pos - 0.5, float2(r.x, -r.y)),
			0.5 + dot(pos - 0.5, float2(r.y,  r.x)));

		rpos += float2(cos(layah), sin(layah));

		float3 layer = tex2D(cloud1_samp, rpos).rgb;
		float a = 1 - (i + 1 - f) / 8;
		col = col * (1 - a) + layer * a;
	}

	return float4(col, alpha);
}

technique postprocess {
	pass P0 {
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 pixel();
	}
}
