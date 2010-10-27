
// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;
float4x4 World : WORLD;
float4x4 View : VIEW;
float4x4 Rotation : ROTATION;

float2 dir = {0.0, 0.0};
#define SAMPLE_COUNT 8
const float alpha[SAMPLE_COUNT] = { 0.06f, 0.15f, 0.3f, 0.6f, 0.6f, 0.3f, 0.15f, 0.06f };

// textures
texture tri_tex;

sampler tri_sampler = sampler_state
{
	Texture = (tri_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

struct VS_OUTPUT 
{
   float4 Position : POSITION0;
   float2 Texcoord : TEXCOORD0;
};

float sub;

float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 color = 0;
	float2 tex = In.Texcoord;
	for (int i = 0; i < SAMPLE_COUNT; i++)
	{
		color += tex2D(tri_sampler, tex) * alpha[i];
		tex += dir;
	}
	return max(color - sub, 0.0);
}

VS_OUTPUT vertex(VS_OUTPUT Input)
{
   VS_OUTPUT Output;

   Output.Position = mul( Input.Position, Rotation );
   Output.Position = mul( Output.Position, WorldViewProjection );
//   Output.Position = mul( Input.Position, WorldViewProjection );
	 Output.Texcoord = float2(Output.Position.x * 0.5 + 0.5f + (0.5f / 256) - 4 * dir.x, -Output.Position.y * 0.5 + 0.5f + (0.5f / 256) - 4 * dir.y);
   
   return( Output );
}


technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
