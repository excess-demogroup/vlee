float4x4 matView : VIEW;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;

const float bend;
const float anim;

texture pattern1_tex;
sampler pattern1_samp = sampler_state {
	Texture = (pattern1_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

texture pattern2_tex;
sampler pattern2_samp = sampler_state {
	Texture = (pattern2_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

struct VS_INPUT {
	float4 Position : POSITION0;
	float3 Normal : NORMAL;
	float2 Uv : TEXCOORD0;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float3 Normal : TEXCOORD0;
	float3 ViewPos : TEXCOORD1;
	float2 Uv1 : TEXCOORD2;
	float2 Uv2 : TEXCOORD3;
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
	float3 pos = Input.Position.xyz;
	pos.y += pow(max(pos.z, 0), 2) * bend;
	Output.Position = mul(float4(pos, 1), matWorldViewProjection);
	Output.Normal = mul(matWorldViewInverse, float4(Input.Normal, 0)).xyz;
	Output.ViewPos = mul(float4(pos, 1), matWorldView).xyz;
	Output.Uv1 = float2(Input.Uv.x, Input.Uv.y * 0.1 + anim);
	Output.Uv2 = float2(Input.Uv.x, Input.Uv.y * 0.1 - anim);
	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	PS_OUTPUT o;
	o.col  = tex2D(pattern1_samp, Input.Uv1);
	o.col += tex2D(pattern2_samp, Input.Uv2);
	o.z = Input.ViewPos.z;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
		CullMode = None;
	}
}
