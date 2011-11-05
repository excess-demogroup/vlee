float4x4 matView : VIEW;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;
const float lum;

struct VS_INPUT {
	float4 Position : POSITION0;
	float3 Normal : NORMAL;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float3 Normal : TEXCOORD0;
	float3 Pos2 : TEXCOORD2;
	float  dist : TEXCOORD4;
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	float3 pos = Input.Position.xyz + float3(0, 100, 0);

	VS_OUTPUT Output;
	Output.dist = length(Input.Position / 100);

	Output.Position = mul(float4(pos, 1), matWorldViewProjection);
	Output.Normal = mul(matWorldViewInverse, float4(Input.Normal, 0));
	Output.Pos2 = mul(Input.Position, matView);

	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	float3 n = normalize(Input.Normal);

	PS_OUTPUT o;
	o.col = lum / (1 + Input.dist / 2);
	o.col.rgb *= 1.25 - pow(max(0, dot(-normalize(Input.Pos2), n)), 1) * 0.5;
	o.z = Input.Pos2.z;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
