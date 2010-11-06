const float flash, fade, scroll;
const float2 noffs;
const float bloom_amt, blur_amt, noise_amt;

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

texture scroller_tex;
sampler scroller = sampler_state {
	Texture = (scroller_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = TRUE;
};

texture noise_tex;
sampler noise = sampler_state {
	Texture = (noise_tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

float loking1_alpha, loking2_alpha;

texture loking1_tex;
sampler loking1 = sampler_state {
	Texture = (loking1_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = TRUE;
};

texture loking2_tex;
sampler loking2 = sampler_state {
	Texture = (loking2_tex);
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
	float4 color;
	color = lerp(tex2D(tex_sampler, In.tex), tex2D(bloom_sampler, In.tex), blur_amt);
	color += pow(tex2D(bloom_sampler, In.tex) * bloom_amt, 1.5);

	float4 s = tex2D(scroller, In.tex * float2(1, 720.0 / 2048) + float2(0, scroll));
	color.rgb *= 1 - s.a;
	color.rgb += s.rgb * s.a;

	if (loking1_alpha > 1e-10) {
		float3 loking = lerp(tex2D(loking1, In.tex).rgb, tex2D(loking2, In.tex).rgb, loking2_alpha);
		color.rgb = lerp(color.rgb, loking, loking1_alpha);
	}

	color.rgb = color.rgb * fade + flash;

	float n = tex2D(noise, In.tex * 15 + noffs).r;
	color.rgb += (n - 0.5) * 0.01 * noise_amt;

	return color;
}

technique color_map {
	pass P0 {
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}

float3 rgbe_to_rgb(float4 rgbe)
{
	return rgbe.rgb * exp2(rgbe.a * 255 - 128);
}

float4 pixel_rgbe(VS_OUTPUT In) : COLOR
{
	float3 color = rgbe_to_rgb(tex2D(tex_sampler, In.tex));
//	color += pow(tex2D(bloom_sampler, In.tex) * 0.75, 1.5);
	float n = tex2D(noise, In.tex * 15 + noffs).r;
	color.rgb += (n - 0.5) * 0.1;
	return float4(color * fade + flash, 1);
}

technique rgbe {
	pass P0 {
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel_rgbe();
	}
}
