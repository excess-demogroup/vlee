
// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;

struct VS_OUTPUT 
{
   float4 pos : POSITION0;
};


float4 pixel(VS_OUTPUT In) : COLOR
{
	float4 col = float4(1.0f,1.0f,1.0f,1.0f);
	return col;
}

VS_OUTPUT vertex(VS_OUTPUT In)
{
    VS_OUTPUT Out;

    Out.pos = mul(In.pos, WorldViewProjection);
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
