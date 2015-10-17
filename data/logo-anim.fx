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

texture greetings_tex;
sampler greetings_samp = sampler_state {
	Texture = (greetings_tex);
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Clamp;
	AddressV = Clamp;
	sRGBTexture = True;
};

float4 ps_main(PS_INPUT Input) : COLOR
{
	float d = tex2D(shape_samp, Input.TexCoord0).x;
	float4 g = tex2D(greetings_samp, float2(Input.TexCoord0.x, 1 - Input.TexCoord0.y));
	return (tex2D(stroke_samp, float2(d, time)) + g) * fade;
}

technique mesh {
	pass Geometry {
		PixelShader  = compile ps_2_0 ps_main();
		AlphaBlendEnable = False;
		ZEnable = False;
	}
}
