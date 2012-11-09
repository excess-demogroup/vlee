float4x4 matView : VIEW;
float4x4 matWorld : WORLD;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;

textureCUBE env_tex;
samplerCUBE env_samp = sampler_state {
	Texture = (env_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	sRGBTexture = TRUE;
};

textureCUBE cube_noise_tex;
samplerCUBE cube_noise_samp = sampler_state {
	Texture = (cube_noise_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	sRGBTexture = FALSE;
};

texture3D volume_noise_tex;
sampler3D volume_noise_samp = sampler_state {
	Texture = (volume_noise_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	AddressW = WRAP;
	sRGBTexture = FALSE;
};

struct VS_INPUT {
	float3 Normal : TEXCOORD0;
	float3 Pos2 : TEXCOORD1;
	float3 UVW : TEXCOORD2;
	float2 UV : TEXCOORD3;
	float3 Pos3 : TEXCOORD4;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float3 Normal : TEXCOORD0;
	float3 Pos2 : TEXCOORD1;
	float3 UVW : TEXCOORD2;
	float2 UV : TEXCOORD3;
	float3 Pos3 : TEXCOORD4;
};

float time;

float3 tpos(float phi, float th, float R, float r)
{
	r += sin(phi * 4 + time) * sin(th * 8 + time) * 2;
	
//	r *= 1 - pow(frac(th / (2 * 3.1415926) + time), 10.0) * 0.3;
	return float3(
		R * cos(th) + r * cos(th) * cos(phi),
		R * sin(th) + r * sin(th) * cos(phi),
		r * sin(phi));
}

VS_OUTPUT vs_main( VS_INPUT Input )
{
	VS_OUTPUT Output;

	float th = Input.UV.y * 3.1415926 * 2;
	float phi = Input.UV.x * 3.1415926 * 2;
	float R = -70;
	float r = 20;
	float eps = 0.01;
	float3 pos = tpos(phi, th, R, r);
	float3 p1 = tpos(phi + eps, th, R, r);
	float3 p2 = tpos(phi, th + eps, R, r);
	float3 n = -normalize(cross(p1 - pos, p2 - pos));

	Output.Position = mul( float4(pos, 1), matWorldViewProjection );
	Output.Normal = mul(matWorldViewInverse, float4(n, 0)).xyz;
	//Output.Normal = n;
	Output.Pos2 = mul( float4(pos, 1), matView ).xyz;
	Output.Pos3 = pos;
	Output.UVW = pos / 25;
	Output.UV = Input.UV;

	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

float3 perturb_normal(float3 norm, float3 grad)
{
	float3 surf_grad = grad - norm * dot(norm, grad);
	return normalize(norm - surf_grad);
}

float3 grad(sampler3D s, float3 uvw, float e)
{
	return float3(
		tex3D(s, uvw + float3(e, 0, 0)).r - tex3D(s, uvw - float3(e, 0, 0)).r,
		tex3D(s, uvw + float3(0, e, 0)).r - tex3D(s, uvw - float3(0, e, 0)).r,
		tex3D(s, uvw + float3(0, 0, e)).r - tex3D(s, uvw - float3(0, 0, e)).r
		) / e;
}

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	PS_OUTPUT o;

	float s = tex3D(volume_noise_samp, Input.UVW).r;
	s = 0.02 + saturate((s - 0.45) * 25) * 0.04;
	float3 g = s * grad(volume_noise_samp, Input.UVW, 1.0 / 128);
	float3 n = perturb_normal(normalize(Input.Normal), g);

	o.z = Input.Pos2.z;
	float3 d = normalize(-Input.Pos2);
	
	float3 lpos = float3(-cos(time * 2), sin(time * 2), 0) * 70;
	lpos = mul(float4(lpos, 1), matView).xyz;
	
	o.col = float4(0, 0, 0, 1);

	float diff = 1 - pow(max(0, dot(d, n)), 0.1);
//	o.col = float4(float3(diff, diff, diff) * float3(0.5, 0.5, 1), 1);
//	o.col.rgb *= 1 + pow(frac(Input.UV.y + time), 10.0) * float3(2, 2, 1) * 10;

	float3 l = normalize(lpos - Input.Pos2);
	float n_dot_l = dot(l, n);
	if (n_dot_l > 0) {
		float3 h = normalize(l + normalize(-Input.Pos2));
		float3 lcol = n_dot_l * 2.5;
		lcol += pow(max(0, dot(n, h)), 256.0) * 5.0;
		lcol *= max(0, 1.005 / (distance(Input.Pos2, lpos) * 2.0) - 0.005);
		o.col.rgb += lcol;
	}

	o.col.rgb = lerp(float3(0.15, 0.2, 0.3), o.col.rgb, exp(-Input.Pos2.z * 0.0005));
//	o.col.rgb = pow(0.5 + 0.5 * n, 2.2);
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_main();
	}
}
