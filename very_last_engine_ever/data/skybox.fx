float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorld : WORLD;
float4x4 matView : VIEW;

uniform float4 vViewPosition;

struct VS_INPUT 
{
	float3 Pos:      POSITION;
};

struct VS_OUTPUT 
{
	float4 Pos:     POSITION;
	float3 texcoord : TEXCOORD0;
};

VS_OUTPUT vs_main( VS_INPUT In )
{
	VS_OUTPUT Out;

	Out.Pos           = mul(float4(In.Pos,  1.0), matWorldViewProjection);
	Out.texcoord      = In.Pos;

	return Out;
}

#define PS_INPUT VS_OUTPUT

#if 0
textureCUBE reflectionMap;
samplerCUBE reflectionMapSampler = sampler_state
{
	Texture = (reflectionMap);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
};
#else
// textures
texture reflectionMap 
< 
    string type = "CUBE";
    string name = "test_cube.dds";
>;

samplerCUBE reflectionMapSampler = sampler_state
{
	Texture = (reflectionMap);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
};
#endif

struct PS_OUTPUT 
{
   float4 color    : COLOR0;
};

PS_OUTPUT ps_main( PS_INPUT In )
{
	PS_OUTPUT Out;

	Out.color = texCUBE(reflectionMapSampler, normalize(In.texcoord));
	return Out;
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
