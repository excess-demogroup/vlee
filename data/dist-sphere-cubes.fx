float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;

struct VS_INPUT {
	float4 Position : POSITION0;
	float4 Normal : NORMAL;
	float4x4 InstanceTransform : TEXCOORD0;
	float3 Color : COLOR;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float3 Normal : TEXCOORD1;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output;

	float3 pos = mul(input.InstanceTransform, float4(input.Position.xyz - 0.5, 1)).xyz;
	output.Position = mul(pos, matWorldViewProjection);

//	output.Normal = mul(float4(input.Normal.xyz, 0), matWorldView).xyz;
	output.Normal = mul(float4(0, 0, 1, 0), matWorldView).xyz;

	return output;
}

struct PS_OUTPUT {
	float4 gbuffer0 : COLOR0;
	float4 gbuffer1 : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	PS_OUTPUT o;

	float3 eyeNormal = normalize(Input.Normal);

	float3 albedo = float3(1, 1, 1);
	float ao = 1;
	float spec = 1;

	o.gbuffer0 = float4(eyeNormal, spec);
	o.gbuffer1 = float4(albedo, 1 - ao);
	return o;
}

technique dist_sphere_cubes {
	pass Geometry {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_main();

		AlphaBlendEnable = False;
		ZWriteEnable = True;
	}
}
