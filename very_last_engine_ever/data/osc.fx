struct VS_OUTPUT
{
	float4 pos  : POSITION;
};

VS_OUTPUT vertex(float4 ipos : POSITION, float2 tex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos = ipos;
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	return float4(1,1,1,1);
}

technique homo
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
