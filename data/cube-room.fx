float4x4 matView : VIEW;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;

texture diff_tex;
sampler diff_samp = sampler_state {
	Texture = (diff_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = TRUE;
};

texture ao_tex;
sampler ao_samp = sampler_state {
	Texture = (ao_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = TRUE;
};

texture norm_tex;
sampler norm_samp = sampler_state {
	Texture = (norm_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

texture spec_tex;
sampler spec_samp = sampler_state {
	Texture = (spec_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

struct VS_INPUT {
	float4 pos : POSITION0;
	float3 normal : NORMAL;
	float3 tangent : TEXCOORD1;
	float3 binormal : TEXCOORD2;
	float2 uv : TEXCOORD0;
};

struct VS_OUTPUT {
	float4 pos : POSITION0;
	float2 uv : TEXCOORD0;
	float3 viewPos : TEXCOORD1;
	float3x3 TangentToView : TEXCOORD2;
};

float3 viewLightPosition;

VS_OUTPUT vs_main(VS_INPUT i)
{
	VS_OUTPUT o;
	o.pos = mul(i.pos, matWorldViewProjection);
	o.uv = i.uv;
	o.viewPos = mul(i.pos, matWorldView).xyz;
	o.TangentToView[0] = mul(float4(i.tangent, 0), matWorldView).xyz;
	o.TangentToView[1] = mul(float4(i.binormal, 0), matWorldView).xyz;
	o.TangentToView[2] = mul(float4(i.normal, 0), matWorldView).xyz;
	return o;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT i)
{
	float3 albedo = tex2D(diff_samp, i.uv * 15).rgb;

	float3 normal = tex2D(norm_samp, i.uv * 15).xyz * 2 - 1;
	normal = normalize(mul(normal, i.TangentToView));

	float ao = tex2D(ao_samp, i.uv).r;

	float3 col = ao * albedo * float3(0.8, 0.85, 1) * 0.6;
	float3 light = normalize(viewLightPosition - i.viewPos);
	float s = pow(tex2D(spec_samp, i.uv * 15).r, 2) * 40;

	float n_dot_l = dot(normal, light);
	if (0 < n_dot_l) {
		float3 view = normalize(-i.viewPos);
		float3 hvec = normalize(light + view);

		float3 diffuse = albedo * n_dot_l;
		float specular = pow(max(0, dot(normal, hvec)), 512.0) * s;
//		specular *= fresnel(view, hvec, 0.0025) * 250;
		col += ao * (diffuse + n_dot_l * specular) * float3(1, 1, 0.8);
	}

	PS_OUTPUT o;
	o.col = float4(col, 1);
	o.z = i.viewPos.z;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
