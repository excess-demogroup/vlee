float alpha = 1.f;
float4x4 transform;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorld : WORLD;
float4x4 matView : VIEW;
float4x4 matViewProjection : VIEWPROJECTION;
float4 viewPosition : VIEWPOSITION;


texture tex;
sampler tex_sampler = sampler_state
{
	Texture = (tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

struct VS_INPUT 
{
   float4 pos  : POSITION;
   float3 normal : NORMAL;
   float3 tangent : TANGENT;
   float3 binormal: BINORMAL;
   float2 uv : TEXCOORD0;
};

struct VS_OUTPUT 
{
   float4 pos : POSITION;
   float2 uv : TEXCOORD0;
   float3 l : TEXCOORD1;
   float3 h : TEXCOORD2;
};

float3 lightPos = {0, 0, 140.0};

VS_OUTPUT vs_main( VS_INPUT input )
{
	VS_OUTPUT output;
	output.pos = mul(input.pos, matWorldViewProjection);
	output.uv = input.uv * 4;
	
	float3x3 tangentSpace = float3x3(input.tangent, input.binormal, input.normal);

	float3 lightVec = lightPos - input.pos;
	output.l = mul(tangentSpace, lightVec);
	
	float3 viewVec = viewPosition.xyz - input.pos;
	float3 halfVec = viewVec + lightPos;
	output.h = mul(tangentSpace, halfVec);
	
   return output;
}

float4 ps_main(
   float2 uv : TEXCOORD0,
   float3 l : TEXCOORD1_centroid,
   float3 h : TEXCOORD2_centroid
) : COLOR0
{
//	float3 n = normalize(tex2D(normalmap, uv).xyz * 2 - 1);
	float3 n = float3(0, 0, 1);
	
	float n_dot_l = dot(normalize(l), n);
	float dl = max(n_dot_l, 0);
	float sl = pow(max(dot(normalize(h), n), 0), 32);
//	sl *= 1-saturate((0.5 - n_dot_l) * 600); // "soft clipping" of the specular
	sl *= 1-saturate((-n_dot_l) * 5); // "soft clipping" of the specular
	
	float3 color = dl * float3(0.5, 0.5, 0.4);
	color += sl * float3(1, 0.9, 0.75) * 4;
	color += 0.1;
	
	return float4(color, 1.0f );
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
