float4x4 matView : VIEW;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;

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

texture env_details_tex;
samplerCUBE env_details_samp = sampler_state {
	Texture = (env_details_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	AddressW = CLAMP;
	sRGBTexture = TRUE;
};

struct VS_INPUT {
	float4 Position : POSITION0;
	float3 Normal : NORMAL;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float3 Normal : TEXCOORD0;
	float4 Pos2 : TEXCOORD2;
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
	float3 pos = Input.Position * 10;
	pos.y = -pos.y;
	Output.Position = mul(float4(pos, 1), matWorldViewProjection);
	Output.Normal = mul(matWorldViewInverse, Input.Normal);
	Output.Pos2 = float4(normalize(pos), mul(float4(pos, 1), matWorldView).z);
	return( Output );
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	PS_OUTPUT o;
	o.col = float4(texCUBE(env_samp, -Input.Pos2.xyz).rgb, 1);
	o.col.rgb *= texCUBE(env_details_samp, -Input.Pos2.xyz).rgb;
	o.col.rgb *= 0.6 + texCUBE(env_details_samp, Input.Pos2.yzx).rgb * 0.4;
	o.col.rgb = lerp(o.col.rgb, float3(0.1, 0.1, 0.1) + o.col.rgb * float3(0.3, 1.7, 1.5), 0.5);
	o.z = Input.Pos2.w;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
