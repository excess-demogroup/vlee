float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;

const float3 amt;
const float3 scale;
const float3 phase;

struct VS_INPUT {
	float4 Position : POSITION0;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
	float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float2 TexCoord0 : TEXCOORD0;
	float2 TexCoord1 : TEXCOORD1;
	float3x3 TangentToView : TEXCOORD2;
};

float3 deform(float3 p)
{
	float s = 3;
	float3 p2 = p * scale + phase;
	s += sin(p2.x) * amt.x;
	s += sin(p2.y) * amt.y;
	s += sin(p2.z) * amt.z;
	return p * s / 3;
}

VS_OUTPUT vs_main(VS_INPUT input)
{
	VS_OUTPUT output;

	float3 pos = input.Position.xyz;
	float3 p1 = pos + input.Tangent * 0.1;
	float3 p2 = pos + input.Binormal * 0.1;

	pos = deform(pos);
	p1 = deform(p1);
	p2 = deform(p2);

	float3 t = normalize(p1 - pos);
	float3 b = normalize(p2 - pos);
	float3 n = cross(t, b);

	output.Position = mul(float4(pos, 1), matWorldViewProjection);
	output.TexCoord0 = input.TexCoord * 5;
	output.TexCoord1 = input.TexCoord;
	output.TangentToView[0] = mul(float4(t, 0), matWorldView).xyz;
	output.TangentToView[1] = mul(float4(b, 0), matWorldView).xyz;
	output.TangentToView[2] = mul(float4(n, 0), matWorldView).xyz;
//	output.TangentToView[2] = input.Tangent; // DEBUG!
	return output;
}

struct PS_OUTPUT {
	float4 gbuffer0 : COLOR0;
	float4 gbuffer1 : COLOR1;
};

texture albedo_tex;
sampler albedo_samp = sampler_state {
	Texture = (albedo_tex);
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Wrap;
	AddressV = Wrap;
	sRGBTexture = True;
};

texture normal_tex;
sampler normal_samp = sampler_state {
	Texture = (normal_tex);
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Wrap;
	AddressV = Wrap;
	sRGBTexture = False;
};

texture specular_tex;
sampler specular_samp = sampler_state {
	Texture = (specular_tex);
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Wrap;
	AddressV = Wrap;
	sRGBTexture = True;
};

texture ao_tex;
sampler ao_samp = sampler_state {
	Texture = (ao_tex);
	MipFilter = Linear;
	MinFilter = Linear;
	MagFilter = Linear;
	AddressU = Wrap;
	AddressV = Wrap;
	sRGBTexture = False;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	PS_OUTPUT o;

#if 1
	float3 eyeNormal = normalize(Input.TangentToView[2]);
#else
	float3 tangentNormal = normalize(tex2D(normal_samp, Input.TexCoord0).xyz * 2 - 1);
	tangentNormal = lerp(tangentNormal, float3(0, 0, 1), 0.75);
	float3 eyeNormal = normalize(mul(tangentNormal, Input.TangentToView));
#endif

	float3 albedo = tex2D(albedo_samp, Input.TexCoord0).rgb;
	float ao = tex2D(ao_samp, Input.TexCoord1).r;
	float spec = 0.5; // tex2D(specular_samp, Input.TexCoord0).r * 5;

	o.gbuffer0 = float4(eyeNormal, spec);
	o.gbuffer1 = float4(albedo, 1 - ao);
	return o;
}

technique dist_sphere {
	pass Geometry {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_main();

		AlphaBlendEnable = False;
		ZWriteEnable = True;
	}
}
