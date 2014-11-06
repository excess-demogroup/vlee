float4x4 matWorld : WORLD;
float4x4 matWorldInverse : WORLDINVERSE;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;

texture env_tex;
samplerCUBE env_samp = sampler_state {
	Texture = (env_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
	sRGBTexture = TRUE;
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

float3 color;

struct VS_INPUT {
	float4 Position : POSITION0;
	float3 Normal : NORMAL;
};

struct VS_OUTPUT {
	float4 ClipPosition : POSITION;
	float4 WorldPosition : TEXCOORD0;
	float3 WorldNormal : TEXCOORD1;
	float3 EyeNormal : TEXCOORD2;
	float4 EyePosition : TEXCOORD3;
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
	float3 pos = Input.Position;

	Output.WorldPosition = mul(float4(pos, 1), matWorld);
	Output.WorldNormal = mul(matWorldInverse, Input.Normal);

	Output.EyePosition = mul(float4(pos, 1), matWorldView);
	Output.EyeNormal = mul(matWorldViewInverse, Input.Normal);

	Output.ClipPosition = mul(float4(pos, 1), matWorldViewProjection);

	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

float3 sample_spectrum(samplerCUBE tex, float3 start, float3 stop, int samples)
{
	float3 delta = (stop - start) / samples;
	float3 pos = start;
	float3 sum = 0, filter_sum = 0;
	for (int i = 0; i < samples; ++i) {
		float3 sample = texCUBE(tex, pos).rgb;
		float t = (i + 0.5) / samples;
		float3 filter = tex2Dlod(spectrum_samp, float4(t, 0, 0, 0)).rgb;
		sum += sample * filter;
		filter_sum += filter;
		pos += delta;
	}
	return sum / filter_sum;
}

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	PS_OUTPUT o;

	float3 view = normalize(Input.WorldPosition - matWorldViewInverse[3].xyz);
	float3 reflection = reflect(view, Input.WorldNormal);
	float3 refraction0 = reflect(view, Input.WorldNormal * 0.39f);
	float3 refraction1 = reflect(view, Input.WorldNormal * 0.41f);

	o.col = float4(lerp(sample_spectrum(env_samp, refraction0, refraction1, 8).rgb,
	                    texCUBE(env_samp, reflection).rgb,
	                    saturate(1.0 + Input.EyeNormal.z)), 1);

//	o.col = float4(reflection, 1);
//	o.col.rgb *= (1.0 + Input.EyeNormal.z);
	o.z = Input.EyePosition.z;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_main();
	}
}
