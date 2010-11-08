const float line_amt;
const float time;
const float pulse_amt;
const float pulse_phase;
const float radial_amt, radial_amt2;

texture noise_tex;
sampler noise = sampler_state {
	Texture = (noise_tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};


texture light1_tex;
float light1_alpha;
sampler light1 = sampler_state {
	Texture = (light1_tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

texture light2_tex;
float light2_alpha;
sampler light2 = sampler_state {
	Texture = (light2_tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

texture cos_tex;
sampler cos_samp = sampler_state {
	Texture = (cos_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

struct VS_OUTPUT {
	float4 pos       : POSITION0;
	float2 texCoord  : TEXCOORD0;
	float2 snakeCoord : TEXCOORD1;
};

VS_OUTPUT vs_main(float4 pos: POSITION, float2 texCoord: TEXCOORD0)
{
	VS_OUTPUT o;
	o.pos = pos;
	o.texCoord = texCoord;

	o.snakeCoord.x = 0.25 + (texCoord.y + texCoord.x * 30 + time * 0.02) / (2.0 * 3.1415926);
	o.snakeCoord.y = (texCoord.x + texCoord.y * 30 + time * 0.02) / (2.0 * 3.1415926);
	return o;
}

float4 ps_main(VS_OUTPUT i) : COLOR
{
	float2 v = float2(
	    tex1D(cos_samp, i.snakeCoord.x).r * 2 - 1,
	    tex1D(cos_samp, i.snakeCoord.y).r * 2 - 1);

	v = pow(v, 1000);
	float3 c = (float3(0.5, 0.5, 1.7) * smoothstep(0.1, 0.9, v.x) +
	            float3(1.0, 0.5, 1.7) * smoothstep(0.1, 0.9, v.y)) * line_amt;

	float d = distance(i.texCoord, float2(0.5, 0.5)) * 4;
	float att = 0.2 / (1 + d);
	float tmp = time / 16;
	c += float3(0.5, 0.5, 1.7) * pow(frac(d - tmp), 64) * att * radial_amt;
	c += float3(1.0, 0.5, 1.7) * pow(frac(d - tmp - 0.5), 64) * att * radial_amt2;

	c += tex2D(light1, i.texCoord) * light1_alpha;
	c += tex2D(light2, i.texCoord) * light2_alpha;
	c = c + c * sin((pulse_phase + tex2D(noise, i.texCoord).r) * 2 * 3.1415926) * pulse_amt;
	return float4(c, 1);
}

technique cube_tops {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
