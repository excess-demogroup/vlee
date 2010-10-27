string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;
float4x4 World : WORLD;
float4x4 View : VIEW;

float4x4 spherelight_transform;

float fade = 1.0;
float overbright = 1.0;
float3 fog_color = float3(1, 1, 1);


float faceLight[6] = {0, 1, 0, 0, 0, 0};

// textures
texture diffuse;
sampler tex_samp = sampler_state
{
	Texture = (diffuse);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MaxAnisotropy = 8;
};

texture lightmap;
sampler lightmap_samp = sampler_state
{
	Texture = (lightmap);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MaxAnisotropy = 8;
};

textureCUBE spherelight;
samplerCUBE spherelight_samp = sampler_state
{
	Texture = (spherelight);
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
	float4 pos : POSITION;
	float3 tex : TEXCOORD0;
	float3 l   : TEXCOORD1;
};

int getFace(float3 v)
{
	float saxis[3] = { v.x,  v.y,  v.z };
	float axis[3] = { abs(v.x), abs(v.y), abs(v.z) };
	int maxAxis;
	if (axis[0]>axis[1] && axis[0]>axis[2]) maxAxis=0;
		else if (axis[1] > axis[2]) maxAxis=1;
		else maxAxis=2;
	if (saxis[maxAxis]<0) maxAxis+=3;	
	return maxAxis;
}


VS_OUTPUT vertex(
	float3 ipos  : POSITION,
	float3 inorm : NORMAL,
	float3 itex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	
	Out.pos = mul(float4(ipos,  1), WorldViewProjection);
	Out.l   = mul(float4(ipos,  1), spherelight_transform).xyz;
	Out.tex = itex;
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float light = texCUBE(spherelight_samp, In.l).r / length(In.l * 0.1);
	light *= faceLight[getFace(In.l)];
	light += 0.5;
	return tex2D(lightmap_samp, In.tex) * tex2D(tex_samp, In.tex * 16) * light;
}

technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
