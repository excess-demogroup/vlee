const float flash, fade, overlay_alpha;
const float2 noffs, nscale;
const float noise_amt;
const float dist_amt, dist_freq, dist_time;
const float2 viewport;
const float color_map_lerp;
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
sampler spectrum_samp2 = sampler_state {
	Texture = (spectrum_tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = WRAP;
	AddressV = WRAP;
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

texture lensdirt_tex;
sampler2D lensdirt_samp = sampler_state {
	Texture = (lensdirt_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
	sRGBTexture = TRUE;
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

float3 sample_bloom(float2 pos)
{
	float3 bloom = tex2Dlod(bloom_samp, float4(pos, 0, 0)) * bloom_weight[0];
	bloom += tex2Dlod(bloom_samp, float4(pos, 0, 1)) * bloom_weight[1];
	bloom += tex2Dlod(bloom_samp, float4(pos, 0, 2)) * bloom_weight[1];
	bloom += tex2Dlod(bloom_samp, float4(pos, 0, 3)) * bloom_weight[3];
	bloom += tex2Dlod(bloom_samp, float4(pos, 0, 4)) * bloom_weight[4];
	bloom += tex2Dlod(bloom_samp, float4(pos, 0, 5)) * bloom_weight[5];
	bloom += tex2Dlod(bloom_samp, float4(pos, 0, 6)) * bloom_weight[6];
	return bloom;
}

float3 sample_lensflare(float2 pos)
{
	// do the lens flare
	float2 ipos = -pos + 1.0;
	const int ghosts = 8;
	float2 delta = (0.5 - ipos) / (ghosts * 0.5);

	float3 flare = 0;
	for (int i = 0; i < ghosts; ++i) {
		float2 ghost_start = ipos + delta * 0.95 * i;
		float2 ghost_stop = ipos + delta * 1.05 * i;
		int ghost_samples = max(3, int(length(viewport * (ghost_stop - ghost_start) / 4)));
		flare += sample_spectrum(bloom_samp, ghost_start, ghost_stop, ghost_samples, 2);
	}

	// fake anamorphic
	float2 nnpos = pos - 0.5;
	float2 nipos = -pos + 0.5;
	flare += tex2Dlod(bloom_samp, float4(nnpos * float2(0.5, 1) + 0.5, 0, 2)).rgb * float3(0.25, 0.25, 2.0);
	flare += tex2Dlod(bloom_samp, float4(nipos * float2(0.3,-1) + 0.5, 0, 2)).rgb * float3(0.25, 0.25, 2.0);

	// sample halo
	float flare_fade = pow(1 - abs(2 * distance(pos, 0.5) - 1), 5);
	float2 halo_start = ipos + normalize(delta) * 0.5 * 0.95;
	float2 halo_stop = ipos + normalize(delta) * 0.5 * 1.05;
	int halo_samples = max(3, int(length(viewport * (halo_stop - halo_start) / 2)));

	flare += sample_spectrum(bloom_samp, halo_start, halo_stop, halo_samples, 2) * flare_fade;

	return flare;
}

float4 pixel(VS_OUTPUT In, float2 vpos : VPOS) : COLOR
{
	float2 block = floor(vpos / 16);
	float2 uv_noise = block / 64;
	uv_noise += floor(dist_time * float2(1234.0, 3543.0)) / 64;

	float block_thresh = pow(frac(dist_time * 1236.0453), 2.0) * 0.2;
	float line_thresh = pow(frac(dist_time * 2236.0453), 3.0) * 0.7;

	float2 pos = In.uv;
	pos += float2(
			(sin(pos.y * dist_freq + dist_time) * 2 - 1) * dist_amt,
			(sin(pos.x * dist_freq + dist_time) * 2 - 1) * dist_amt);

	float dist = pow(2 * distance(In.uv, float2(0.5, 0.5)), 2);
	const float sep = 0.03;
	float2 end = (pos - 0.5) * (1 - dist * sep * 2) + 0.5;

	// glitch some blocks and lines
	if (tex2D(noise_samp, uv_noise).r < block_thresh ||
	    tex2D(noise_samp, float2(uv_noise.y, 0)).r < line_thresh) {
	    float2 dist = (frac(uv_noise) - 0.5) * 0.3;
		pos += dist * 0.1;
		end += dist * 0.12;
	}

	int samples = max(3, int(length(viewport * (end - pos) / 2)));
	float3 col = sample_spectrum(color_samp, pos, end, samples, 0);

	col += (sample_bloom(pos) + sample_lensflare(pos)) * tex2Dlod(lensdirt_samp, float4(pos, 0, 0));

	col = color_correct(col);

	// blend overlay
	float4 o = tex2D(overlay_samp, In.uv);
	col = lerp(col, o.rgb, o.a * overlay_alpha);

	// loose luma for some blocks
	if (tex2D(noise_samp, -uv_noise).r < block_thresh)
		col.rgb = col.ggg;

	// discolor block lines
	if (tex2D(noise_samp, float2(uv_noise.y, 0.0)).r * 2.5 < line_thresh)
		col.rgb = float3(0, dot(col.rgb, float3(1, 1, 1)), 0);

	// interleave lines in some blocks
	if (tex2D(noise_samp, float2(0.5 - uv_noise.x, uv_noise.y)).r * 1.5 < block_thresh ||
	    tex2D(noise_samp, float2(0, uv_noise.y)).r * 2.5 < line_thresh)
	{
		col.rgb *= tex2Dlod(spectrum_samp2, float4(vpos.y / 3, 0, 0, 0)).rgb * 1.25;
	}

	// apply flashes
	col = col * fade + flash;

	// a tad of noise makes everything look cooler
	col += (tex2D(noise_samp, In.uv * nscale + noffs) - 0.5) * (1.0 / 8);

	return float4(col, 1);
}

technique postprocess {
	pass P0 {
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 pixel();
	}
}
