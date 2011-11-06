float4x4 matView : VIEW;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;
float pulse;

texture3D noise_tex;
sampler3D noise_samp = sampler_state {
	Texture = (noise_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	AddressW = WRAP;
	sRGBTexture = FALSE;
};

textureCUBE env_tex;
samplerCUBE env_samp = sampler_state {
	Texture = (env_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	sRGBTexture = TRUE;
};

struct VS_INPUT {
	float4 Position : POSITION0;
	float3 Normal : NORMAL;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float3 Normal : TEXCOORD0;
	float3 WorldNormal : TEXCOORD1;
	float3 Pos2 : TEXCOORD2;
	float3 uvw : TEXCOORD3;
	float  dist : TEXCOORD4;
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
	VS_OUTPUT Output;

	float3 pos = Input.Position;
//	Output.dist = pulse - length(Input.Position / 100) - 0.61;
	Output.dist = (pulse - 0.5) - (length(Input.Position / 100) - 0.62);

	if (Output.dist > -0.5) {
		float bulge = pow(abs(frac(Output.dist) - 0.5) * 2, 10);
		pos += Input.Normal * bulge * 2;
	}

	Output.Position = mul(float4(pos, 1), matWorldViewProjection);
	Output.Normal = mul(matWorldViewInverse, float4(Input.Normal, 0));
	Output.WorldNormal = Input.Normal;
	Output.Pos2 = mul(Input.Position, matWorldView);
	Output.uvw = Input.Position / 10;

	return Output;
}

float3 perturb_normal(float3 norm, float3 grad)
{
	float3 surf_grad = grad - norm * dot(norm, grad);
	return normalize(norm - surf_grad);
}

float3 grad(sampler3D s, float3 uvw, float e)
{
	return float3(
		tex3D(s, uvw + float3(e, 0, 0)).r - tex3D(s, uvw - float3(e, 0, 0)).r,
		tex3D(s, uvw + float3(0, e, 0)).r - tex3D(s, uvw - float3(0, e, 0)).r,
		tex3D(s, uvw + float3(0, 0, e)).r - tex3D(s, uvw - float3(0, 0, e)).r
		) / e;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	float s = tex3D(noise_samp, Input.uvw / 10).r;
	s = 0.02 + saturate((s - 0.45) * 25) * 0.02;
	float3 g = s * grad(noise_samp, Input.uvw, 1.0 / 128);
	float3 n = perturb_normal(normalize(Input.Normal), g);
	float3 wn = perturb_normal(normalize(Input.WorldNormal), g);

	PS_OUTPUT o;
	o.col = float4(texCUBE(env_samp, -wn).rgb, 1);
	o.col.rgb = lerp(o.col.rgb, float3(0.1, 0.1, 0.1) + o.col.rgb * float3(0.3, 1.7, 1.5), 0.5) * 2;

	o.col.rgb *= 1.1 - pow(max(0, dot(-normalize(Input.Pos2), n)), 1);
	if (Input.dist > -0.5)
		o.col.rgb += pow(abs(frac(Input.dist) - 0.5) * 2, 10) * float3(1, 0.25, 0.1);

	o.z = Input.Pos2.z;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
