float4x4 matView : VIEW;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;

float fade;
texture tex;
sampler tex_samp = sampler_state {
	Texture = (tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = TRUE;
};

struct VS_INPUT {
	float4 Position : POSITION0;
	float2 Uv : TEXCOORD0;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float2 Uv : TEXCOORD0;
	float4 Pos2 : TEXCOORD2;
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
	float3 pos = Input.Position.xyz * 500;
	Output.Position = mul(float4(pos, 1), matWorldViewProjection);
	Output.Pos2 = float4(normalize(pos), mul(float4(pos, 1), matWorldView).z);
	Output.Uv = Input.Uv;
	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	PS_OUTPUT o;
	o.col = tex2D(tex_samp, Input.Uv) * fade;
	o.z = Input.Pos2.w;
	return o;
}

technique kocka {
	pass P0 {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_main();
		CullMode = None;
		AlphaBlendEnable = True;
		ZWriteEnable = False;
		SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
	}
}
