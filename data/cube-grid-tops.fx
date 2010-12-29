float4x4 matView : WORLDVIEW;
float4x4 matViewProjection : WORLDVIEWPROJECTION;
static const float2 invMapSize = float2(1.0 / 128, 1.0 / 128);
const float2 uv_offs;
const float fog_density;
texture cube_light_tex;

sampler light = sampler_state {
	Texture = (cube_light_tex);
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

struct VS_OUTPUT {
	float4 pos : POSITION0;
	float2 cpos : TEXCOORD0;
	float fog : TEXCOORD4;
};

VS_OUTPUT vs_main(float4 ipos : POSITION0)
{
	VS_OUTPUT o;
	o.pos = mul(ipos, matViewProjection);
	o.cpos = (floor(ipos.xz / 16) + 0.5) * invMapSize;
	o.cpos += uv_offs;
	o.cpos.x = 1 - o.cpos.x;
	float eyez = mul(ipos, matView).z;
	o.fog = exp(-(eyez * eyez * fog_density));
	return o;
}

float4 rgb_to_rgbe(float3 rgb)
{
	float e = ceil(log2(max(max(rgb.r, rgb.g), rgb.b)));
	return float4(rgb * exp2(-e), (e + 128) / 255);
}

float4 ps_main(VS_OUTPUT i, uniform bool rgbe) : COLOR0
{
	float3 c = tex2D(light, i.cpos).rgb * 2.5;
	float ao = 0.005;
	c = (ao + c) * i.fog;
	return rgbe ? rgb_to_rgbe(c) : float4(c, 1.0);
}

technique cube_tops {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main(false);
	}
}

technique rgbe {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main(true);
	}
}
