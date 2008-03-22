string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

float alpha = 1.f;
float xoffs = 0.f;
float yoffs = 0.f;

float xzoom = 1.f;
float yzoom = 1.f;
float4x4 tex_transform;
float4x4 spherelight_transform;

float3 up;
float3 left;
float3 fog_color = float3(1, 1, 1);

float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;

// textures
texture tex;

float faceLight[6] = {0, 1, 0, 0, 0, 0};
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


sampler tex_samp = sampler_state
{
	Texture = (tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

textureCUBE spherelight;
samplerCUBE spherelight_samp = sampler_state
{
	Texture = (spherelight);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MaxAnisotropy = 8;
};

struct VS_INPUT
{
	float3 pos  : POSITION;
	float  size : TEXCOORD1;
	float2 tex  : TEXCOORD2;
};

struct VS_OUTPUT
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD0;
	float4 l   : TEXCOORD1;
	int axis : TEXCOORD2;
};

VS_OUTPUT vertex(VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.l.xyz = mul(float4(In.pos / 10,  1), spherelight_transform).xyz;
	Out.l.w = length(Out.l.xyz);
	Out.axis = getFace(Out.l);
	
	In.pos += left * In.tex.x * In.size;
	In.pos += up   * In.tex.y * In.size;
	
	Out.pos   = mul(float4(In.pos,  1), WorldViewProjection);
	
	Out.tex = float2(In.tex.x * 0.5, -In.tex.y * 0.5);
	Out.tex += float2(0.5f, 0.5f);
	
	return Out;
}

struct PS_OUTPUT
{
	float4 col : COLOR;
};

PS_OUTPUT pixel(VS_OUTPUT In)
{
	PS_OUTPUT output;
	
	float light = (texCUBE(spherelight_samp, In.l.xyz).r * faceLight[In.axis]) / In.l.w;
	light *= 2;
	light += 0.025;

	output.col = tex2D(tex_samp, In.tex) * light * alpha;
	return output;
}

technique particle_yes_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
