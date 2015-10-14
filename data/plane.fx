float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;

struct VS_INPUT {
	float4 Position : POSITION0;
	float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float2 TexCoord0 : TEXCOORD0;
};

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Position = mul(input.Position, matWorldViewProjection);
	output.TexCoord0 = input.TexCoord;
	return output;
}

texture albedo_tex;
sampler albedo_samp = sampler_state {
	Texture = (albedo_tex);
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Wrap;
	AddressV = Wrap;
	sRGBTexture = True;
};

float4 ps_main(VS_OUTPUT Input) : COLOR
{
	return tex2D(albedo_samp, Input.TexCoord0);
}

technique mesh {
	pass Geometry {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();

		AlphaBlendEnable = True;
		SrcBlend = SrcAlpha;
		DestBlend = One;

		CullMode = None;

		ZEnable = True;
		ZWriteEnable = False;
	}
}
