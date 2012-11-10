const float alpha;
const float3 up;
const float3 left;
const float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
const float4x4 WorldView : WORLDVIEW;

// textures
texture tex;
sampler tex_samp = sampler_state {
	Texture = (tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

struct VS_INPUT {
	float3 pos  : POSITION;
	float  size : TEXCOORD0;
	float2 uv   : TEXCOORD1;
};

struct VS_OUTPUT {
	float4 pos : POSITION;
	float2 uv  : TEXCOORD0;
	float  alpha : TEXCOORD1;
};

VS_OUTPUT vertex(VS_INPUT In)
{
	VS_OUTPUT Out;
	In.pos += left * In.uv.x * In.size;
	In.pos += up   * In.uv.y * In.size;
	Out.pos = mul(float4(In.pos,  1), WorldViewProjection);
	Out.uv = (In.uv + 1) / 2;
	Out.uv.y = 1 - Out.uv.y;
	Out.alpha = 1.0 / (1 + mul(float4(In.pos,  1), WorldView).z * 0.1); // In.size / 15;
	return Out;
}

float4 white_pixel(VS_OUTPUT In)  : COLOR
{
	return tex2D(tex_samp, In.uv) * In.alpha * alpha;
}

technique white {
	pass P0 {
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 white_pixel();
		AlphaBlendEnable = True;
		ZWriteEnable = False;
		SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
	}
}
