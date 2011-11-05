float4x4 matView : VIEW;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;

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

struct VS_INPUT {
	float4 Position : POSITION0;
	float3 Normal : NORMAL;
	float2 uv : TEXCOORD0;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float3 Pos2 : TEXCOORD2;
	float2 uv : TEXCOORD3;
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
	VS_OUTPUT Output;
	float3 pos = Input.Position.xyz;
	Output.Position = mul(float4(pos, 1), matWorldViewProjection);
	Output.Pos2 = mul(Input.Position, matWorldView);
	Output.uv = Input.uv;
	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	PS_OUTPUT o;
	float ao = tex2D(ao_samp, Input.uv).r;
	o.col = float4(ao, ao, ao, 1) * 0.1;
	// cheap-ass fog
	o.col.rgb = lerp(o.col.rgb, 0.1, saturate(length(Input.Pos2) / 50));
	o.z = Input.Pos2.z;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
