string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

float faceLight[6] = {0, 1, 0, 0, 0, 0};

// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;
float4x4 World : WORLD;
float4x4 View : VIEW;

float4x4 spherelight_transform;

float fade = 1.0;
float overbright = 1.0;
float3 fog_color = float3(1, 1, 1);


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

struct VS_OUTPUT
{
	float4 pos : POSITION;
	float3 tex : TEXCOORD0;
	float on : TEXCOORD1;
};

VS_OUTPUT vertex(
	float3 ipos  : POSITION,
	float3 inorm : NORMAL,
	float3 itex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos = mul(float4(ipos,  1), WorldViewProjection);
	Out.tex = itex;
	Out.on = faceLight[getFace(ipos)];
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	const float ambient = 0.1;
	float4 col = tex2D(lightmap_samp, In.tex) * In.on;
	return (0.075 + col) * 3;
}

technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
