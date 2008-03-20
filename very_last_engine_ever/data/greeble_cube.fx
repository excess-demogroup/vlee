string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;
float4x4 World : WORLD;
float4x4 View : VIEW;
float4 vViewPosition : VIEWPOSITION;

float fade = 1.0;
float overbright = 1.0;
float3 fog_color = float3(1, 1, 1);


// textures
texture lightmap;
sampler lightmap_samp = sampler_state
{
	Texture = (lightmap);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MaxAnisotropy = 8;
};

textureCUBE env;
samplerCUBE env_samp = sampler_state
{
	Texture = (env);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MaxAnisotropy = 8;
};

struct VS_OUTPUT 
{
	float4 pos        : POSITION0;
	float2 tex        : TEXCOORD0;
	float3 reflection : TEXCOORD1;
};

VS_OUTPUT vertex(
	float3 ipos  : POSITION,
	float3 inorm : NORMAL,
	float3 itex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos  = mul(float4(ipos,  1), WorldViewProjection);
	
	float3 view = ipos - vViewPosition.xyz;
	float3 normal = inorm;

	Out.reflection = reflect(view, normal);
	Out.tex = itex;
	
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float ao = tex2D(lightmap_samp, In.tex).r;
	float4 color = ao * 0.4;
	
	float4 ref = texCUBE(env_samp, In.reflection);
	float gloss = ao;
	gloss *= 0.75 + tex2D(lightmap_samp, In.tex.yx * 10).r / 4;
	gloss *= 0.50 + tex2D(lightmap_samp, In.tex.yx * 40).r / 2;
	gloss *= gloss;
	color += ref * gloss * 0.25;
	
	return color;
}

technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
