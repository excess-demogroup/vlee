const float2 viewport;

texture bloom_tex;
sampler bloom_samp = sampler_state {
	Texture = (bloom_tex);
	MipFilter = POINT;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = BORDER;
	AddressV = BORDER;
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

float3 sample_spectrum(sampler2D tex, float2 start, float2 stop, int samples, float lod)
{
	float2 delta = (stop - start) / samples;
	float2 pos = start;
	float3 sum = 0, filter_sum = 0;
	for (int i = 0; i < samples; ++i) {
		float3 sample = tex2Dlod(tex, float4(pos, 0, lod)).rgb;
		float t = (i + 0.5) / samples;
		float3 filter = tex2Dlod(spectrum_samp, float4(t, 0, 0, 0)).rgb;
		sum += sample * filter;
		filter_sum += filter;
		pos += delta;
	}
	return sum / filter_sum;
}

float3 sample_lensflare(float2 pos)
{
	// do the lens flare
	float2 ipos = -pos + 1.0;
	const int ghosts = 8;
	float2 delta = (0.5 - ipos) / (ghosts * 0.5);

	float3 flare = 0;
	for (int i = 0; i < ghosts; ++i) {
		float2 ghost_start = ipos + delta * 0.8 * i;
		float2 ghost_stop = ipos + delta * 1.2 * i;
		int ghost_samples = clamp(int(length(viewport * (ghost_stop - ghost_start) / 16)), 3, 8);
		flare += sample_spectrum(bloom_samp, ghost_start, ghost_stop, ghost_samples, 1);
	}

	// fake anamorphic
	float2 nnpos = pos - 0.5;
	float2 nipos = -pos + 0.5;
	flare += tex2Dlod(bloom_samp, float4(nnpos * float2(0.5, 1) + 0.5, 0, 1)).rgb * float3(0.25, 0.25, 2.0);
	flare += tex2Dlod(bloom_samp, float4(nipos * float2(0.3,-1) + 0.5, 0, 1)).rgb * float3(0.25, 0.25, 2.0);

	// sample halo
	float flare_fade = pow(1 - abs(2 * distance(pos, 0.5) - 1), 5);
	float2 halo_start = ipos + normalize(delta) * 0.5 * 0.95;
	float2 halo_stop = ipos + normalize(delta) * 0.5 * 1.05;
	int halo_samples = clamp(int(length(viewport * (halo_stop - halo_start) / 2)), 3, 8);

	flare += sample_spectrum(bloom_samp, halo_start, halo_stop, halo_samples, 2) * flare_fade;

	return flare;
}

float4 pixel(VS_OUTPUT In, float2 vpos : VPOS) : COLOR
{
	return float4(sample_lensflare(In.uv), 1);
}

technique postprocess {
	pass P0 {
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 pixel();
		ZEnable = False;
	}
}
