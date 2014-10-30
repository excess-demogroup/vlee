float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldInverse : WORLDINVERSE;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;

float3 fogColor;
float fogDensity;

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
	float4x4 InstanceTransform : TEXCOORD0;
	float3 Color : COLOR;
};

struct VS_OUTPUT {
	float4 ClipPos : POSITION0;
	float3 ViewPos : TEXCOORD1;
	float3 Color : TEXCOORD2;
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
	VS_OUTPUT Output;

	// perform instance-to-model-space (yeahyeah, double model-space - sue me)
	float3 Position = mul(Input.InstanceTransform, float4(Input.Position.xyz - 0.5, 1)).xyz;

	Output.ClipPos = mul(float4(Position, 1), matWorldViewProjection);
	Output.ViewPos = mul(float4(Position, 1), matWorldView).xyz;
	Output.Color = Input.Color;
	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	PS_OUTPUT o;
	o.col = float4(Input.Color, 1);
	o.col.rgb = lerp(fogColor, o.col.rgb, exp(-Input.ViewPos.z * fogDensity));
	o.z = Input.ViewPos.z;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_main();
		AlphaBlendEnable = False;
		ZWriteEnable = True;
		CullMode = None;
	}
}
