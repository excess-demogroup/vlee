
// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 World : WORLD;
float4x4 Projection : PROJECTION;
float4x4 View : VIEW;
float4x4 worldpos;
float3 stop;

float dstep;
float time;

// textures
texture explosion_tex;

sampler explosion_sampler = sampler_state
{
	Texture   = (explosion_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU  = CLAMP;
	AddressV  = CLAMP;
};

struct VS_OUTPUT 
{
   float4 pos : POSITION0;
   float2 tex : TEXCOORD0;
};
struct VS_INPUT 
{
   float4 pos     : POSITION0;
   float2 tex     : TEXCOORD0;
   float3 dir     : TEXCOORD1;
   float3 initPos : TEXCOORD2;
   float index    : TEXCOORD3;
   float size     : TEXCOORD4;
   float weight   : TEXCOORD5;
};


float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 col = tex2D(explosion_sampler, In.tex);
	return float4(col.xyz, col.a * 0.5);
}

VS_OUTPUT vertex(VS_INPUT In)
{
    VS_OUTPUT Out;

    Out.pos = mul(In.pos, worldpos)+float4(In.dir, 0.0)*(dstep*(time+In.index));
    Out.pos = mul(Out.pos, View);
    Out.pos = mul(Out.pos, Projection);

    Out.tex = float2(In.tex.x * 0.5, -In.tex.y * 0.5);
    Out.tex += float2(0.5f, 0.5f);   
    return(Out);
}


technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
