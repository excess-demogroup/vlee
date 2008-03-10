string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

float fade  = 0.75f;
float sobel_fade = 0.5f;

float alpha = 1.f;
float xoffs = 0.f;
float yoffs = 0.f;

float xzoom = 1.f;
float yzoom = 1.f;

float flash = 0.0f;
float fade2 = 1.0f;

// textures
texture tex;
texture color_map;

sampler tex_sampler = sampler_state
{
	Texture = (tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

texture tex2;
sampler tex2_sampler = sampler_state
{
	Texture = (tex2);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

texture desaturate;
sampler desaturate_sampler = sampler_state
{
	Texture = (desaturate);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
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
	float4 color =
		tex2D(tex_sampler, In.tex) * alpha
		+ tex2D(tex2_sampler, In.tex) * 0.5 ;
	
	/* lookup in palette */
	float lum = luminance(color.rgb);
//	float lum = (color.r + color.g + color.b) / 3;
	
	float pal_sel = tex2D(desaturate_sampler, In.tex);
//	float pal_sel = 1 - length(In.tex - 0.5) * 1.5;
	
	float3 pal_color = tex2D(color_map_sampler, float2(lum, pal_sel)).rgb;
	color.rgb = lerp(color.rgb, pal_color, fade);

//	float3 delta = color - lum;
//	color.rgb += delta * pal_sel;
	return color + flash;
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
