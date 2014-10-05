const float flash, fade, overlay_alpha;
const float2 noffs, nscale;
const float noise_amt;
const float dist_amt, dist_freq, dist_time;
const float2 viewport;
const float color_map_lerp;
const float bloom_amt;
const float bloom_weight[7];

texture color_tex;
sampler color_samp = sampler_state {
	Texture = (color_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

texture bloom_tex;
sampler bloom_samp = sampler_state {
	Texture = (bloom_tex);
	MipFilter = POINT;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

texture spectrum_tex;
sampler spectrum_samp = sampler_state {
	Texture = (spectrum_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

texture noise_tex;
sampler noise_samp = sampler_state {
	Texture = (noise_tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

texture overlay_tex;
sampler overlay_samp = sampler_state {
	Texture = (overlay_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = TRUE;
};

texture color_map1_tex;
sampler3D color_map1_samp = sampler_state {
	Texture = (color_map1_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
	sRGBTexture = FALSE;
};

texture color_map2_tex;
sampler3D color_map2_samp = sampler_state {
	Texture = (color_map2_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
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

float3 sample_spectrum(float2 start, float2 stop, int samples)
{
	float2 delta = (stop - start) / samples;
	float2 pos = start;
	float3 sum = 0, filter_sum = 0;
	for (int i = 0; i < samples; ++i) {
		float3 sample = tex2Dlod(color_samp, float4(pos, 0, 0)).rgb;
		float t = (i + 0.5) / samples;
		float3 filter = tex2Dlod(spectrum_samp, float4(t, 0, 0, 0));
		sum += sample * filter;
		filter_sum += filter;
		pos += delta;
	}
	return sum / filter_sum;
}

float3 color_correct(float3 color)
{
#if 0
	float3 uvw = pow(color, 1.0 / 2.2) * (31.0 / 32) + 0.5 / 32;
	return lerp(tex3Dlod(color_map1_samp, float4(uvw, 0)),
                tex3Dlod(color_map2_samp, float4(uvw, 0)),
                color_map_lerp);
#else
	return sqrt(color);
#endif
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	const float sep = 0.03;
	float2 pos = In.uv;
	pos += float2(
			(sin(pos.y * dist_freq + dist_time) * 2 - 1) * dist_amt,
			(sin(pos.x * dist_freq + dist_time) * 2 - 1) * dist_amt);

	float dist = pow(2 * distance(In.uv, float2(0.5, 0.5)), 2);
	float2 end = (pos - 0.5) * (1 - dist * sep * 2) + 0.5;

	int samples = max(3, int(length(viewport * (end - pos) / 2)));
	float3 col = sample_spectrum(pos, end, samples);

	float3 bloom = tex2Dlod(bloom_samp, float4(In.uv, 0.0, 0.0)) * bloom_weight[0];
	bloom += tex2Dlod(bloom_samp, float4(In.uv, 0.0, 1.0)) * bloom_weight[1];
	bloom += tex2Dlod(bloom_samp, float4(In.uv, 0.0, 2.0)) * bloom_weight[1];
	bloom += tex2Dlod(bloom_samp, float4(In.uv, 0.0, 3.0)) * bloom_weight[3];
	bloom += tex2Dlod(bloom_samp, float4(In.uv, 0.0, 4.0)) * bloom_weight[4];
	bloom += tex2Dlod(bloom_samp, float4(In.uv, 0.0, 5.0)) * bloom_weight[5];
	bloom += tex2Dlod(bloom_samp, float4(In.uv, 0.0, 6.0)) * bloom_weight[6];

	col += bloom * bloom_amt;
	col = color_correct(col);

	float4 o = tex2D(overlay_samp, In.uv);
	col.rgb = lerp(col.rgb, o.rgb, o.a * overlay_alpha);

	col = col * fade + flash;

	col += (tex2D(noise_samp, In.uv * nscale + noffs) - 0.5) * (1.0 / 8);

	return float4(col, 1);
}

technique postprocess {
	pass P0 {
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 pixel();
	}
}
