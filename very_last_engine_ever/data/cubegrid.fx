string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;
float4x4 World : WORLD;
float4x4 View : VIEW;

texture front_tex;
sampler3D front_sampler = sampler_state
{
	Texture = (front_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = MIRRORONCE;
	AddressV = MIRRORONCE;
	AddressW = CLAMP;
};

texture side_tex;
sampler3D side_sampler = sampler_state
{
	Texture = (side_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = MIRRORONCE;
	AddressV = CLAMP;
	AddressW = CLAMP;
};

struct VS_OUTPUT
{
	float4 pos  : POSITION;
	half   fog  : TEXCOORD0;
	half2  uv   : TEXCOORD1;
	half   front_dist : TEXCOORD2;
	half   light : TEXCOORD3;
};

const float3 normals[6] =
{
	float3( 0.0,  0.0,  1.0),
	float3( 0.0,  0.0, -1.0),
	float3( 0.0,  1.0,  0.0),
	float3( 0.0, -1.0,  0.0),
	float3( 1.0,  0.0,  0.0),
	float3(-1.0,  0.0,  0.0)
};

VS_OUTPUT vertex(
	/* mesh data */
	float4 ipos  : POSITION,
	float4 uv_face_index : TEXCOORD0, /* xy = uv, z = undefined, w = face index */
	
	/* instance data */
	float4 ipos2 : TEXCOORD1,
	float4 distances[2]  : TEXCOORD2
	)
{
	/* calculate object-space position */
	float3 pos = (ipos.xyz - float3(0.5, 0.5, 0.5)) * (ipos2.w / 255);
	
	int face_index = uv_face_index.w * 256;
	float3 normal = normals[face_index];

	VS_OUTPUT Out;
	Out.pos = mul(float4(pos + ipos2.xyz,  1), WorldViewProjection);

/*	Out.uv = ipos */
	Out.uv = (uv_face_index.xy - 0.5) * (ipos2.w / 255) + 0.5;
	
	float sizes[6] = {
		distances[0].r,
		distances[0].g,
		distances[0].b,
		distances[0].a,
		distances[1].r,
		distances[1].g,
	};
	
	Out.front_dist = sizes[face_index];
	
	Out.light = max(dot(normal, normalize(float3(1,0.1,0.2))), 0);
	Out.fog = clamp((Out.pos.z - 10) / 70, 0.0, 1.0);

	return Out;
}

float4x4 hatch_transform;
float4 pixel(VS_OUTPUT In, float2 vpos : VPOS) : COLOR
{
//	float4 color = float4(In.uv,1,1);
	
	float front_dist = In.front_dist;
	
	float u = In.uv.x;
	float v = In.uv.y;
	
	float ao = tex3D(front_sampler, float3(u * 2 - 1, v * 2 - 1, 1.0 - front_dist)).x;
	
	float l = (In.light * ao) + (ao * 0.1);
	
	float4 color = float4(l, l, l, 1.0);
	
/*	float2 hatch = mul(vpos, hatch_transform);
	float hatch_amt = frac(hatch.x + hatch.y) > 0.5 ? 0.0 : 1.0;
	color.g += hatch_amt; */
	
	return color;
//	return lerp(color, float4(0,0,0,0), In.fog);
}

technique schvoi
{
	pass P0
	{
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 pixel();
	}
}
