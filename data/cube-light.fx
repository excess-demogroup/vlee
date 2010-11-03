float time;
float pulse_amt;

texture noise_tex;
sampler noise = sampler_state {
	Texture = (noise_tex);
	MipFilter = POINT;
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
	MipFilter = POINT;
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
	MipFilter = POINT;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = FALSE;
};

struct VS_OUTPUT {
	float4 pos       : POSITION0;
	float2 texCoord  : TEXCOORD0;
};

VS_OUTPUT vs_main(float4 pos: POSITION, float4 texCoord: TEXCOORD0)
{
	VS_OUTPUT o = (VS_OUTPUT) 0;
	o.pos = pos;
	o.pos.x += 1.0 / 128;
	o.pos.y -= 1.0 / 128;
	o.texCoord = texCoord;
	return o;
}

float4 ps_main(float2 texCoord : TEXCOORD0) : COLOR
{
	float2 v = float3(
	    sin(texCoord.y + texCoord.x * 30 + time * 0.2),
	    cos(texCoord.x + texCoord.y * 30 + time * 0.2),
	    0);
	v = pow(v, 1000);
	float3 c = float3(0.5, 0.5, 1.7) * smoothstep(0.1, 0.9, v.x) * 2 +
	           float3(1.0, 0.5, 1.7) * smoothstep(0.1, 0.9, v.y) * 2;
	c *= 0.2;
	c += tex2D(light1, texCoord) * light1_alpha;
	c += tex2D(light2, texCoord) * light2_alpha;
	c = c + c * sin(time + tex2D(noise, texCoord)) * pulse_amt;
	return float4(c, 1);
}

technique cube_tops {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
