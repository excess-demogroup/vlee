string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;
float4x4 World : WORLD;
float4x4 View : VIEW;

struct VS_OUTPUT
{
	float4 pos  : POSITION;
};

VS_OUTPUT vertex(
	float3 ipos  : POSITION,
	float3 ipos2  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos  = mul(float4(ipos + ipos2,  1), WorldViewProjection);
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	return float4(1, 1, 1, 1);
}

technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
