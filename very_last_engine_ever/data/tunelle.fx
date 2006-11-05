string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;
float4x4 World : WORLD;
float4x4 View : VIEW;

float fade = 1.0;
float overbright = 1.0;

// textures
texture map;
sampler tex_samp = sampler_state
{
	Texture = (map);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MaxAnisotropy = 8;
};

texture env
< 
	string type = "CUBE";
>;


sampler env_samp = sampler_state
{
	Texture = (env);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
};

struct VS_OUTPUT
{
	float4 pos  : POSITION;
	float3 norm : TEXCOORD0;
	float3 tex  : TEXCOORD1;
	float3 pos2 : TEXCOORD2;
	float  fog_factor : TEXCOORD3;
};

VS_OUTPUT vertex(
	float3 ipos  : POSITION,
	float3 inorm : NORMAL,
	float3 itex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos  = mul(float4(ipos,  1), WorldViewProjection);
	Out.pos2 = Out.pos.xyz;
	
	Out.norm = mul(inorm, WorldView);
	Out.tex = itex; // Out.pos;

	Out.fog_factor = 1 - clamp(1.0f / (Out.pos.z / 10), 0.0, 1.0);
	
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 color;
	float3 N = normalize(In.norm);
	float3 L = normalize(In.pos2);
	
//	float3 ref_vec = reflect(N, L);
	float3 ref_vec = reflect(L, N);

	color = max(dot(-N, L), 0);
	color += texCUBE(env_samp, ref_vec) * pow(1 - max(dot(-N, L), 0), 1);
	color *= tex2D(tex_samp, In.tex);
	color *= 0.25;
	
	float4 fog_color = float4(1, 1, 1, 1);
	return float4(lerp(color.xyz, fog_color.xyz, In.fog_factor), color.a);
	
	return color * overbright;
}

technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
