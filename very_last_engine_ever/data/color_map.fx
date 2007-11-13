string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

float fade = 0.75f;
float sobel_fade = 0.5f;

float alpha = 1.f;
float xoffs = 0.f;
float yoffs = 0.f;

float xzoom = 1.f;
float yzoom = 1.f;

// textures
texture tex;
texture color_map;

sampler tex_sampler = sampler_state
{
	Texture = (tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

sampler color_map_sampler = sampler_state
{
	Texture = (color_map);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
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

float texel_width = 1.f / 800;
float texel_height = 1.f / 600;

float4 sobel(sampler samp, float2 uv)
{
	float4 color1 = float4(0, 0, 0, 0);
	color1 += tex2D(tex_sampler, uv + float2(-texel_width, -texel_height)) * -1;
	color1 += tex2D(tex_sampler, uv + float2(-texel_width, 0)) * -2;
	color1 += tex2D(tex_sampler, uv + float2(-texel_width, +texel_height)) * -1;
	color1 += tex2D(tex_sampler, uv + float2(+texel_width, -texel_height)) *  1;
	color1 += tex2D(tex_sampler, uv + float2(+texel_width, 0)) *  2;
	color1 += tex2D(tex_sampler, uv + float2(+texel_width, +texel_height)) * 1;

	float4 color2 = float4(0, 0, 0, 0);
	color2 += tex2D(tex_sampler, uv + float2(-texel_width, -texel_height)) * -1;
	color2 += tex2D(tex_sampler, uv + float2(0,            -texel_height)) * -2;
	color2 += tex2D(tex_sampler, uv + float2(+texel_width, -texel_height)) * -1;
	color2 += tex2D(tex_sampler, uv + float2(-texel_width, +texel_height)) *  1;
	color2 += tex2D(tex_sampler, uv + float2(0,            +texel_height)) *  2;
	color2 += tex2D(tex_sampler, uv + float2(+texel_width, +texel_height)) * 1;
	
	float val = sqrt(color1.x * color1.x + color2.x * color2.x);
	return float4(val, val, val, val);
}

float luminance(float3 color)
{
	return
		(color.r * 0.299) +
		(color.g * 0.587) +
		(color.b * 0.114);
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 tex = tex2D(tex_sampler, In.tex);
	float4 edges = sobel(tex_sampler, In.tex);
	float4 color1 = lerp(tex, edges, sobel_fade);
	
	float lum = luminance(color1.rgb);
	float4 color2 = tex2D(color_map_sampler, lum);
	return lerp(color1, color2, fade);
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
