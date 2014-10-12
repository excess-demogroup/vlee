const float3 up;
const float3 left;
const float4x4 matView : VIEW;
const float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;

struct VS_INPUT {
	float3 pos  : POSITION;
	float  size : TEXCOORD0;
	float2 uv   : TEXCOORD1;
};

struct VS_OUTPUT {
	float4 pos : POSITION;
	float2 uv  : TEXCOORD0;
	float4 color : TEXCOORD1;
};

float focal_distance, focal_length, f_stop;
float2 viewport;

float coc(float z)
{
	float p = focal_distance; // focal distance
	float f = focal_length; // focal length
	float F = f_stop; // F-stop
	return ((z * f) / (z - f) - (p * f) / (p - f)) * (p - f) / (p * F);
}

VS_OUTPUT vertex(VS_INPUT In)
{
	float3 pos = In.pos;
	float eyeDepth = mul(float4(pos, 1), matView).z;
	float4 screenPos = mul(float4(pos,  1), matWorldViewProjection);

	float c = abs(coc(eyeDepth) * viewport.y);
	float pixelSize = c; // + distance(screenPos * 0.5, 0.0);
	pixelSize = max(pixelSize, 3);
	float size = pixelSize / viewport.y;
	
	pos += size * screenPos.w * (In.uv.x * left + In.uv.y * up);

	VS_OUTPUT Out;
	Out.pos = mul(float4(pos,  1), matWorldViewProjection);
	Out.uv = In.uv;
	Out.color = (float4(1,1,1,1) * 10) / (pixelSize * pixelSize);

	if (screenPos.z < 0.5)
		Out.pos.w = -1;
	return Out;
}

float4 pixel(VS_OUTPUT In)  : COLOR
{
   float d = max(abs(In.uv.x), 0.87 * abs(In.uv.y) + 0.5 * abs(In.uv.x)) * 0.6;
   float e = fwidth(d);
   d = saturate((0.5 - d) / e);
   return d * In.color;
}

technique white {
	pass P0 {
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 pixel();
		AlphaBlendEnable = True;
		ZWriteEnable = False;
		SrcBlend = SrcAlpha;
		DestBlend = One;
	}
}
