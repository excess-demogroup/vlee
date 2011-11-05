float4x4 matView : VIEW;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;
float3 viewPos;

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

texture logo_tex;
sampler logo_samp = sampler_state {
	Texture = (logo_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = BORDER;
	AddressV = BORDER;
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
	float3 Pos3 : TEXCOORD3;
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	float3 pos = Input.Position.xyz * 10;

	Output.Position = mul(float4(pos, 1), matWorldViewProjection);
	Output.Normal = mul(matWorldViewInverse, float4(0, 1, 0, 0));
	Output.WorldNormal = float3(0, 1, 0);
	Output.Pos2 = mul(float4(pos, 1), matWorldView);
	Output.Pos3 = pos;

	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	float3 n = normalize(Input.Normal);
	float3 wn = tex2D(norm_samp, Input.Pos3.xz * 0.005).xyz * 2 - 1;
	wn = normalize(wn.xzy);

	float3 pos = Input.Pos3;
	float3 dir = reflect(Input.Pos3 - viewPos, wn);
	float dist = 1.0;
	float t = -(pos.z + dist) / dir.z;
	pos += dir * t;

	PS_OUTPUT o;
	o.col.a = 1;
	o.col.rgb = tex2D(diff_samp, Input.Pos3.xz * 0.005).xyz * 0.4;
//	o.col.rgb = 1.25 - pow(max(0, dot(-normalize(Input.Pos2), n)), 1) * 0.5;
	o.z = Input.Pos2.z;

	float2 uv = float2(1, -1) * ((pos.xy) / 650) + float2(0.5, 0.75);
	o.col.rgb += tex2D(logo_samp, uv);
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
