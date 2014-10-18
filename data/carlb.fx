float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldInverse : WORLDINVERSE;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;

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
	float4 ClipPos : POSITION0;
	float3 ViewPos : TEXCOORD2;
	float3 WorldPos : TEXCOORD3;
	float3 ViewNormal : TEXCOORD4;
	float3 WorldNormal : TEXCOORD5;
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
	VS_OUTPUT Output;

	float3 pos = Input.Position.xyz;

	Output.ClipPos = mul( float4(pos, 1), matWorldViewProjection );
	Output.ViewPos = mul(float4(pos, 1), matWorldView).xyz;
	Output.WorldPos = Input.Position.xyz;
	Output.ViewNormal = mul(matWorldViewInverse, float4(Input.Normal, 0)).xyz;
	Output.WorldNormal = mul(matWorldInverse, float4(Input.Normal, 0)).xyz;
	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
//	float3 n = normalize(cross(ddx(Input.ViewPos), ddy(Input.ViewPos)));
	float3 n = normalize(Input.ViewNormal);

	PS_OUTPUT o;
	float3 uvw = reflect(normalize(Input.WorldNormal), -normalize(Input.ViewPos));
	uvw = n; // mul(matWorldView, float4(uvw, 0)).xyz;
	o.col = float4(texCUBE(env_samp, uvw).rgb * float3(0.5, 0.7, 0.55), 1);
	o.col.rgb *= pow(1 - abs(n.z), 2);

	o.z = Input.ViewPos.z;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_main();
	}
}
