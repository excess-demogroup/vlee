const float time, fade;

struct PS_INPUT {
	float4 Position : POSITION0;
	float2 TexCoord0 : TEXCOORD0;
};

texture shape_tex;
sampler shape_samp = sampler_state {
	Texture = (shape_tex);
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Clamp;
	AddressV = Clamp;
	sRGBTexture = False;
};

texture stroke_tex;
sampler stroke_samp = sampler_state {
	Texture = (stroke_tex);
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Clamp;
	AddressV = Wrap;
	sRGBTexture = True;
};

float4 ps_main(PS_INPUT Input) : COLOR
{
	float d = tex2D(shape_samp, Input.TexCoord0).x;
	return tex2D(stroke_samp, float2(d, time)) * fade;
}

technique mesh {
	pass Geometry {
		PixelShader  = compile ps_2_0 ps_main();
		AlphaBlendEnable = False;
		ZEnable = False;
	}
}
