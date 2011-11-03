const float flash, fade;
const float2 noffs, nscale;
const float bloom_amt, blur_amt, noise_amt;
const float dist_amt, dist_freq, dist_time;

texture bloom;
sampler bloom_sampler = sampler_state {
	Texture = (bloom);
	MipFilter = POINT;
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
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

sampler tex_point_sampler = sampler_state {
	Texture = (tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
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

texture overlay_tex;
sampler overlay = sampler_state {
	Texture = (overlay_tex);
	MipFilter = LINEAR;
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

VS_OUTPUT vertex(float4 ipos : POSITION, float2 tex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos = ipos;
	Out.tex = tex;
	return Out;
}

float3 rgbe_to_rgb(float4 rgbe)
{
	return rgbe.rgb * exp2(rgbe.a * 255 - 128);
}

float4 pixel(VS_OUTPUT In, uniform bool rgbe) : COLOR
{
	float3 color;
	float2 dist = float2(
		(sin(In.tex.y * dist_freq + dist_time) * 2 - 1) * dist_amt,
		(sin(In.tex.x * dist_freq + dist_time) * 2 - 1) * dist_amt);

	float3 tmp = rgbe ?
		rgbe_to_rgb(tex2D(tex_point_sampler, In.tex + dist)) :
		tex2D(tex_sampler, In.tex + dist).rgb;

	color = lerp(tmp, tex2D(bloom_sampler, In.tex + dist).rgb, blur_amt);
	color += pow(tex2D(bloom_sampler, In.tex + dist).rgb * bloom_amt, 1.5);

	if (loking1_alpha > 1e-10) {
		float3 loking = lerp(tex2D(loking1, In.tex).rgb, tex2D(loking2, In.tex).rgb, loking2_alpha);
		color = lerp(color, loking, loking1_alpha);
	}

	float4 o = tex2D(overlay, In.tex);
	color *= 1 - o.a;
	color += o.rgb * o.a;

	color = color * fade + flash;

	float n = tex2D(noise, In.tex * nscale + noffs).r;
	color += (n - 0.5) * noise_amt;

	return float4(color, 1);
}

technique color_map {
	pass P0 {
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel(false);
	}
}

technique rgbe {
	pass P0 {
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel(true);
	}
}
