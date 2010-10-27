string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;
float4x4 World : WORLD;
float4x4 View : VIEW;

float fade = 1.0;

// textures
texture EnvironmentMap 
< 
    string type = "CUBE";
    string name = "test_cube.dds";
>;

sampler Environment = sampler_state
{
	Texture = (EnvironmentMap);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
};

struct VS_OUTPUT
{
	float4 pos  : POSITION;
	float3 norm : TEXCOORD0;
	float3 tex  : TEXCOORD1;
	float  z    : TEXCOORD2;
};

VS_OUTPUT vertex(
	float3 ipos  : POSITION,
	float3 inorm : NORMAL,
	float3 itex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos  = mul(float4(ipos,  1), WorldViewProjection);
	Out.norm = normalize(mul(inorm, WorldView));
	Out.tex = Out.pos;
	Out.z = Out.pos.z;
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 color;

	In.norm = normalize(In.norm);
	
	float3 ref_vec = reflect(normalize(In.tex), In.norm);
	float4 env = texCUBE(Environment, ref_vec);

	color = env;
	color *= 0.05 + pow(1 - abs(In.norm.z), 1) * (1.f / (0.5 + In.z * 0.01));
	color = pow(color, 2.0);
	color.b *= 0.7;

	color = color * fade;
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
