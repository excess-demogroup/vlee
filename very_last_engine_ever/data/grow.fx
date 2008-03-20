
// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;

struct VS_OUTPUT 
{
   float4 pos : POSITION0;
   float2 tex : TEXCOORD0;
};

VS_OUTPUT vertex(VS_OUTPUT In)
{
    VS_OUTPUT Out;

    Out.pos = mul(In.pos, WorldViewProjection);
    Out.tex.x = Out.pos.z;
    Out.tex.y = Out.pos.z;
    return(Out);
}


float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 col = float4(1.1f-(In.tex.x/20.f),1.1f-(In.tex.x/20.f),1.0f-(In.tex.x/20.f),1.5f);
	return col;
}


technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
