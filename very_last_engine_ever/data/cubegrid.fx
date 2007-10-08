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
	float fog : TEXCOORD0;
};

VS_OUTPUT vertex(
	float4 ipos  : POSITION,
	float4 ipos2 : TEXCOORD0,
	float4  distances[2] : TEXCOORD1,
	float4 normal_front_index : TEXCOORD3
	)
{
	/* calculate object-space position */
	float3 pos = (ipos.xyz - float3(0.5, 0.5, 0.5)) * (ipos2.w / 255);
	
	float3 normal = (normal_front_index.xyz - float3(0.5, 0.5, 0.5)) * 2;
	int front_index = normal_front_index.w * 256;
	
	VS_OUTPUT Out;
	Out.pos = mul(float4(pos + ipos2,  1), WorldViewProjection);
//	Out.fog = Out.pos.z / 70;
	
	float sizes[6] = {
		distances[0].r,
		distances[0].g,
		distances[0].b,
		distances[0].a,
		distances[1].r,
		distances[1].g,
	};
	
	Out.fog = sizes[front_index];
	return Out;
}

float4 pixel(VS_OUTPUT In) : COLOR
{
//	return float4(1, 1, 1, 1);
	return lerp(float4(1, 1, 1, 1), float4(0,0,0,0), In.fog);
}

technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_2_0 vertex();
		PixelShader  = compile ps_2_0 pixel();
	}
}
