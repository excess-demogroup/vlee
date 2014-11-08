float4x4 matView : VIEW;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;

float3 fogColor;
float fogDensity;

struct VS_INPUT {
	float4 Position : POSITION0;
	float3 Normal : NORMAL;
	float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float3 Normal : TEXCOORD0;
	float2 TexCoord : TEXCOORD1;
	float4 Pos2 : TEXCOORD2;
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;
	float3 pos = Input.Position;
	Output.Position = mul(float4(pos, 1), matWorldViewProjection);
	Output.Normal = mul(matWorldViewInverse, Input.Normal);
	Output.TexCoord = Input.TexCoord;
	Output.Pos2 = mul(float4(pos, 1), matWorldView);
	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	PS_OUTPUT o;
	float l = 0.75;
	o.col = float4(l, l, l, 1);
	o.col.rgb = lerp(fogColor, o.col.rgb, exp(-Input.Pos2.z * fogDensity));
	o.z = Input.Pos2.z;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
