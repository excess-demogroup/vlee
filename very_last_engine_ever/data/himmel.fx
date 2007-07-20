float alpha = 1.f;

texture tex;
sampler tex_sampler = sampler_state
{
	Texture = (tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = WRAP;
	AddressV = WRAP;
};

float2  disco_offset;
texture disco_tex;
sampler disco_sampler = sampler_state
{
	Texture = (disco_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = WRAP;
	AddressV = WRAP;
};

struct VS_OUTPUT
{
	float4 pos  : POSITION;
	float2 tex  : TEXCOORD1;
};

VS_OUTPUT vertex(float4 ipos : POSITION, float2 tex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos = ipos;
	Out.tex = tex;
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float2 dir = (In.tex - float2(0.42, 0.44));
	dir.x *= 4.f / 3;
	float distance = length(dir);
	float angle  = atan2(dir.x, dir.y) * (1.0 / 3.141592);
//	angle += 2.14159 / 2;
//	return float4(distance, angle, 0, 0);
	float4 himmel = tex2D(tex_sampler, In.tex);
//	float4 disco  = tex2D(disco_sampler, float2(angle * 10, distance) + disco_offset) / ((distance + 0.5) * 3);
	float4 disco  = tex2D(disco_sampler, float2(angle * 10, distance) + disco_offset);
	disco.rg /= ((distance + 0.2) * 10);
//	float4 disco  = tex2D(disco_sampler, float2(distance, angle) + disco_offset);
	return himmel + disco * alpha;
	
//	return lerp(himmel, himmel + disco * alpha, alpha);
//	return lerp(himmel, disco, alpha);

//	return tex2D(tex_sampler, In.tex) * alpha;
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
