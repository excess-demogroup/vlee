const float4x4 matWorldView : WORLDVIEW;
const float4x4 matProjection : PROJECTION;
const float4x4 matProjectionInverse : PROJECTIONINVERSE;

#ifdef FAST_BOUNDS

float4 getSphereBounds(float3 center, float r)
{
	float4 p = center.xyxy + float4(-r, -r, r, r);
	float4 z = center.zzzz - float4(-r, -r, r, r) * sign(p);
	return p * float2(matProjection[0].x, matProjection[1].y).xyxy / z;
}

#else

float4 getSphereBounds(float3 center, float r)
{
	float2 cLengthRcp = float2(dot(center.xz, center.xz),
	                           dot(center.yz, center.yz));
	float2 tSquared = cLengthRcp - r * r;

	if (tSquared.x > 0 && tSquared.y > 0) {
		float3 a = float3(sqrt(tSquared.x), r, -r) * cLengthRcp.x;
		float3 b = float3(sqrt(tSquared.y), r, -r) * cLengthRcp.y;
		float4 p = float4(dot(a.xz, center.xz), dot(b.xz, center.yz),
		                  dot(a.xy, center.xz), dot(b.xy, center.yz));
		float4 z = float4(dot(a.yx, center.xz), dot(b.yx, center.yz),
		                  dot(a.zx, center.xz), dot(b.zx, center.yz));
		return p * float2(matProjection[0].x, matProjection[1].y).xyxy / z;
	}

	return float4(-1, -1, 1, 1); // whole screen
}

#endif

struct VS_INPUT {
	float3 pos  : POSITION;
	float  size : TEXCOORD0;
	float2 uv   : TEXCOORD1;
};

struct VS_OUTPUT {
	float4 pos : POSITION0;
	float3 dir : TEXCOORD0;
	float depth : TEXCOORD1;
	float rr : TEXCOORD2;
	float3 origin : TEXCOORD3;
	float3 spherePos : TEXCOORD4;
};

VS_OUTPUT vertex(VS_INPUT In)
{
	float2 pos = In.uv;
	float3 spherePos = In.pos;
	float sphereRadius = In.size;

	float3 spherePosEye = mul(float4(spherePos, 1), matWorldView).xyz;
	float4 spherePosClip = mul(float4(spherePosEye, 1), matProjection);
	if (spherePosClip.z < -spherePosClip.w) {
		VS_OUTPUT o;
		o.pos = float4(0, 0, 0, -1);
		o.dir = 0;
		o.origin = 0;
		o.depth = 0;
		// return o;
	}

	float4 bbox = getSphereBounds(spherePosEye, sphereRadius);
	pos.x = clamp(pos.x, bbox.x, bbox.z);
	pos.y = clamp(pos.y, bbox.y, bbox.w);

	float4 eyeSpaceNear = mul(float4(pos, 0, 1), matProjectionInverse);
	float4 eyeSpaceFar = mul(float4(pos, 1, 1), matProjectionInverse);
	float3 rayStartEye = eyeSpaceNear.xyz / eyeSpaceNear.w; 
	float3 rayTargetEye = eyeSpaceFar.xyz / eyeSpaceFar.w;

	VS_OUTPUT o;
	o.pos = float4(pos, 0, 1);
	o.dir = rayTargetEye - rayStartEye;
	o.origin = rayStartEye;
	o.depth = rayStartEye.z;
	o.spherePos = spherePosEye;

	// just a silly precalc
	o.rr = sphereRadius * sphereRadius;
	return o;
}

struct PS_OUTPUT {
	float4 color : COLOR;
	float4 viewDepth : COLOR1;
	float depth : DEPTH;
};

PS_OUTPUT pixel(VS_OUTPUT In)
{
	float3 dir = normalize(In.dir);
	float3 pos = In.origin - In.spherePos;
	float rr = In.rr;

	float a = dot(pos, pos);
	float b = dot(pos, dir);
	float d = dot(float3(-a, b, rr), float3(1, b, 1));

	PS_OUTPUT o;
	if (d > 0) {
		float t = -b - sqrt(d);

		// calculate normal
		float3 n = normalize(pos + dir * t);
		o.color = float4(n, 1);

		// calculate z
		float depth = In.depth + dir.z * t;
		float2 clipPos = depth * matProjection[2].zw + matProjection[3].zw;
		o.depth = clipPos.x / clipPos.y;
		o.viewDepth = depth;

		if (0 > o.depth || o.depth > 1)
			discard;
	} else
		discard;

	return o;
}

float sphereAO(float3 pos, float3 normal, float3 spos, float r)
{
	float3 dir = spos.xyz - pos;
	float l  = length(dir);
	float nl = dot(normal, dir / l);
	float h  = l / r;
	float h2 = h * h;
	float k2 = 1 - h2 * nl * nl;

	// above/below horizon: Quilez - http://iquilezles.org/www/articles/sphereao/sphereao.htm
	float res = max(0, nl) / h2;

	// intersecting horizon: Lagarde/de Rousiers - http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
	if (k2 > 0)
	{
#ifdef ACCURATE_AO
		res = nl * acos(-nl * sqrt((h2 - 1) / (1 - nl * nl))) - sqrt(k2 * (h2 - 1));
		res = res / h2 + atan(sqrt(k2 / (h2 - 1)));
		res /= 3.141593;
#else
		// cheap approximation: Quilez
		res = pow(clamp(0.5 * (nl * h + 1) / h2, 0, 1), 1.5);
#endif
	}

	return res;
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

texture gbuffer_tex;
sampler gbuffer_samp = sampler_state {
	Texture = (gbuffer_tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

struct VS_OUTPUT2 {
	float4 pos : POSITION0;
	float3 spherePos : TEXCOORD0;
	float2 sphereRadius : TEXCOORD1;
	float2 uv : TEXCOORD2;
};

const float2 viewport;

VS_OUTPUT2 vertex2(VS_INPUT In)
{
	float2 pos = In.uv;
	float3 spherePos = In.pos;
	float sphereRadius = In.size * 3;

	float3 spherePosEye = mul(float4(spherePos, 1), matWorldView).xyz;
	float4 spherePosClip = mul(float4(spherePosEye, 1), matProjection);
	if (spherePosClip.z < -spherePosClip.w) {
		VS_OUTPUT o;
		o.pos = float4(0, 0, 0, -1);
		o.dir = 0;
		o.origin = 0;
		o.depth = 0;
		// return o;
	}

	float4 bbox = getSphereBounds(spherePosEye, sphereRadius);
	pos.x = clamp(pos.x, bbox.x, bbox.z);
	pos.y = clamp(pos.y, bbox.y, bbox.w);

	VS_OUTPUT2 o;
	o.pos = float4(pos, 0, 1);
	o.spherePos = spherePosEye;
	o.sphereRadius = float2(In.size, sphereRadius);
	o.uv = pos.xy; // * float2(1, -1);
	o.uv += 0.5 / viewport;

	// just a silly precalc
	return o;
}

float4 pixel2(VS_OUTPUT2 In) : COLOR0
{
	float2 texCoord = 0.5 + In.uv * float2(0.5, -0.5);
	float eyeDepth = tex2D(depth_samp, texCoord).r;

	// early out
	if (eyeDepth < In.spherePos.z - In.sphereRadius.y ||
	    eyeDepth > In.spherePos.z + In.sphereRadius.y)
		discard;

	// TODO: optimize this (eye-pos reconstruction, super-silly implementation)
	float4 temp = mul(float4(0, 0, eyeDepth, 1), matProjection);
	float4 temp2 = mul(float4(In.uv, temp.z / temp.w, 1), matProjectionInverse);
	float3 eyePos = temp2.xyz / temp2.w;

	float3 eyeNormal = tex2D(gbuffer_samp, texCoord).xyz;

	float d = distance(eyePos.xyz, In.spherePos);
	if (d > In.sphereRadius.y)
		discard;

	// TODO: optimize this in VS
	float f = 1 - (d - In.sphereRadius.x) / (In.sphereRadius.y - In.sphereRadius.x);

	float ao = sphereAO(eyePos, eyeNormal, In.spherePos, In.sphereRadius.x);

	return float4(0, 0, 0, ao * f);
}

technique sphere {
	pass Geometry {
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 pixel();
		AlphaBlendEnable = False;
		ZWriteEnable = True;
	}

	pass AO {
		VertexShader = compile vs_3_0 vertex2();
		PixelShader  = compile ps_3_0 pixel2();
		ZWriteEnable = False;
		AlphaBlendEnable = True;
		SeparateAlphaBlendEnable = True;
		SrcBlend = One;
		DestBlend = One;
		SrcBlendAlpha = InvDestAlpha;
		DestBlendAlpha = One;
	}
}
