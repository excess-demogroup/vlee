float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;
float4x4 World : WORLD;
float4x4 View : VIEW;
float4 vViewPosition : VIEWPOSITION;


struct VS_OUTPUT 
{
	float4 pos        : POSITION0;
};

VS_OUTPUT vertex(float3 ipos  : POSITION)
{
	VS_OUTPUT Out;
	Out.pos  = mul(float4(ipos,  1), WorldViewProjection);
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	return float4(0, 0, 0, 1);
}

technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
