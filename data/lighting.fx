const float4x4 matProjectionInverse : PROJECTIONINVERSE;
const float4x4 matView : VIEW;
const float4x4 matViewInverse : VIEWINVERSE;
const float3 fogColor;
const float fogDensity;
const float4x4 planeMatrix;
const float3 planeVertices[4];
const float2 nearFar;
const float planeOverbright;

struct VS_OUTPUT {
	float4 pos : POSITION;
	float2 tex : TEXCOORD1;
	float2 dir : TEXCOORD0;
};

VS_OUTPUT vertex(float4 pos : POSITION, float2 tex : TEXCOORD0)
{
	// set up ray
	float4 eyeSpaceNear = mul(float4(pos.xy, 0, 1), matProjectionInverse);
	float4 eyeSpaceFar = mul(float4(pos.xy, 1, 1), matProjectionInverse);
	float3 rayStartEye = eyeSpaceNear.xyz / eyeSpaceNear.w;
	float3 rayTargetEye = eyeSpaceFar.xyz / eyeSpaceFar.w;
	float3 dir = rayTargetEye - rayStartEye;

	VS_OUTPUT o;
	o.pos = pos;
	o.tex = tex;
	o.dir = dir.xy / dir.z;
	return o;
}

texture depth_tex;
sampler depth_samp = sampler_state {
	Texture = (depth_tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

texture gbuffer_tex0;
sampler gbuffer_samp0 = sampler_state {
	Texture = (gbuffer_tex0);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = False;
};

texture gbuffer_tex1;
sampler gbuffer_samp1 = sampler_state {
	Texture = (gbuffer_tex1);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = True;
};

texture logo_tex;
sampler logo_samp = sampler_state {
	Texture = (logo_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = BORDER;
	AddressV = BORDER;
	sRGBTexture = FALSE;
};

float3 planeIntersect(float3 ro, float3 rd, const float4x4 planeMatrix)
{
	float3 p = planeMatrix[3].xyz - ro;
	float3 q = cross(p, rd);
	float u = dot(planeMatrix[1].xyz, q);
	float v = dot(planeMatrix[0].xyz, q);
	float t = dot(planeMatrix[2].xyz, p);
	return float3(u, v, t) / dot(planeMatrix[2].xyz, rd);
}

float4 pixel(VS_OUTPUT In) : COLOR
{
	float2 dir = In.dir;

	float clipDepth = tex2D(depth_samp, In.tex.xy).r;
	float eyeDepth = rcp(clipDepth * nearFar.x + nearFar.y);
	float3 eyePos = float3(dir.xy * eyeDepth, eyeDepth);

	float4 g0 = tex2D(gbuffer_samp0, In.tex.xy);
	float4 g1 = tex2D(gbuffer_samp1, In.tex.xy);

	float3 eyeNormal = g0.rgb;
	float spec = g0.a;
	float3 albedo = g1.rgb;
	float ao = 1 - g1.a;

	float3 col = albedo * ao;

#if 1
	float3 planeVertexDirs[5];
	for (int i = 0; i < 4; ++i)
		planeVertexDirs[i] = normalize(planeVertices[i].xyz - eyePos);
	planeVertexDirs[4] = planeVertexDirs[0];

	float3 lv = float3(0, 0, 0);
	for (i = 0; i < 4; ++i) {
		float3 v0 = planeVertexDirs[i];
		float3 v1 = planeVertexDirs[i + 1];

		float a = acos(dot(v0, v1));
		float3 b = normalize(cross(v0, v1));
		lv += a * b;
	}

#if 0
	// two-sided lighting
	if (dot(planeMatrix[2].xyz, eyePos) < dot(planeMatrix[2].xyz, planeMatrix[3].xyz))
		lv = -lv;
#endif

	float tmp = dot(lv, eyeNormal);
	if (tmp > 0) {
		float factor = tmp / (2 * 3.14159265);
		float3 logo_color = tex2Dlod(logo_samp, float4(0.5, 0.5, 0, 999)).rgb * planeOverbright;
		col += albedo * logo_color * factor;
	}
#endif

	float3 viewDir = normalize(eyePos);
	float3 rayOrigin = eyePos;
	float3 rayDir = reflect(viewDir, eyeNormal);
	float fres = pow(saturate(1 + dot(eyeNormal, viewDir.xyz) * 0.95), 0.25);

	float3 hit = planeIntersect(rayOrigin, rayDir, planeMatrix);
	if (hit.z > 0) {
		float3 refl = tex2Dlod(logo_samp, float4(hit.xy, 0, 0)).rgb;
		col += refl * spec * fres;
	}

	col.rgb = lerp(fogColor, col.rgb, exp(-eyeDepth * fogDensity));

//	return float4(albedo, 1);
//	return float4(ao, ao, ao, 1);
//	return float4(frac(eyePos), 1);
//	return float4((float3)length(eyeNormal), 1);
//	return float4(max(0, eyeNormal), 1);
//	return float4(fres, fres, fres, 1);
//	return float4(abs(matViewInverse[0].xyz), 1);
//	return float4(spec, spec, spec, 1);
//	return frac(100 * tex2D(depth_samp, In.tex.xy).r);
//	return tex2D(depth_samp, float4(In.tex.xy, In.tex.xy * 10 - 5)).r;

	if (length(eyeNormal) < 0.1)
		return float4(fogColor, 1);

	return float4(col, 1);
}

technique lighting {
	pass P0 {
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 pixel();
		ZEnable = False;
		CullMode = None;
	}
}
