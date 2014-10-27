texture color_tex;
sampler color_samp = sampler_state {
	Texture = (color_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = FALSE;
};

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

texture premult_tex;
sampler premult_samp = sampler_state {
	Texture = (premult_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = BORDER;
	AddressV = BORDER;
	sRGBTexture = FALSE;
};

texture temp1_tex;
sampler temp1_samp = sampler_state {
	Texture = (temp1_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = BORDER;
	AddressV = BORDER;
	sRGBTexture = FALSE;
};

texture temp2_tex;
sampler temp2_samp = sampler_state {
	Texture = (temp2_tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = BORDER;
	AddressV = BORDER;
	sRGBTexture = FALSE;
};

float focal_distance, focal_length, f_stop;
float2 viewport;
#define SAMPLES 20

float coc(float z)
{
	float p = focal_distance; // focal distance
	float f = focal_length; // focal length
	float F = f_stop; // F-stop
	return ((z * f) / (z - f) - (p * f) / (p - f)) * (p - f) / (p * F);
}

struct VS_OUTPUT {
	float4 pos  : POSITION;
	float2 tex  : TEXCOORD0;
};

float4 ps_premult(float2 texCoord : TEXCOORD0) : COLOR
{
	float eyeDepth = tex2D(depth_samp, texCoord).r;

	float size = abs(coc(eyeDepth));
	size += (distance(texCoord, 0.5) * 2) * 0.0125;
	size = clamp(size * viewport.y, 2, 20);

	float3 col = tex2D(color_samp, texCoord).rgb;
	return float4(col * size, size);
}

struct TEMP_OUT {
	float4 blur1 : COLOR0;
	float4 blur2 : COLOR1;
};

TEMP_OUT ps_temp(float2 p : TEXCOORD0)
{
	TEMP_OUT o;
	float4 curr = tex2Dlod(premult_samp, float4(p, 0, 0));
	float4 sum = 0;

	float2 s = float2(sin(3.14159265 / 3), -cos(3.14159265 / 3)) / viewport;
	float2 p2 = p + s * 0.5;
	for (int i = 1; i < SAMPLES; ++i) {
		float4 tmp = tex2Dlod(premult_samp, float4(p2, 0, 0));
		float alpha = saturate(tmp.a - i);
		alpha *= saturate(curr.a - i);
		sum += tmp * alpha;
		p2 += s;
	}
	if (sum.a < 1)
		o.blur2 = curr;
	else
		o.blur2 = float4(sum.rgb / sum.a, 1.0) * curr.a;

	s.x = -s.x;
	p2 = p + s * 0.5;
	for (int i = 1; i < SAMPLES; ++i) {
		float4 tmp = tex2Dlod(premult_samp, float4(p2, 0, 0));
		float alpha = saturate(tmp.a - i);
		alpha *= saturate(curr.a - i);
		sum += tmp * alpha;
		p2 += s;
	}

	if (sum.a < 1)
		o.blur1 = curr;
	else
		o.blur1 = float4(sum.rgb / sum.a, 1.0) * curr.a;

	return o;
}

float4 ps_dof(float2 p : TEXCOORD0) : COLOR
{
	float4 curr = tex2Dlod(temp1_samp, float4(p, 0, 0));
	float4 sum = 0;

	float2 s = float2(0, 1) / viewport;
	float2 p2 = p + s * 0.5;
	for (int i = 1; i < SAMPLES; ++i) {
		float4 tmp = tex2Dlod(temp1_samp, float4(p2, 0, 0));
		float alpha = saturate(tmp.a - i);
		alpha *= saturate(curr.a - i);
		sum += 2 * tmp * alpha;
		p2 += s;
	}

	s = float2(-sin(3.14159265 / 3), -cos(3.14159265 / 3)) / viewport;
	p2 = p + s * 0.5;
	for (int i = 1; i < SAMPLES; ++i) {
		float4 tmp = tex2Dlod(temp2_samp, float4(p2, 0, 0));
		float alpha = saturate(tmp.a - i);
		alpha *= saturate(curr.a - i);
		sum += tmp * alpha;
		p2 += s;
	}

	if (sum.a < 1)
		return float4(curr.rgb / curr.a, 1);
	else
		return float4(sum.rgb / sum.a, 1);
}


VS_OUTPUT vs_main(float4 pos : POSITION, float2 tex : TEXCOORD0)
{
	VS_OUTPUT o;
	o.pos = pos;
	o.tex = tex;
	return o;
}

technique dof {
	pass premult {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_premult();
	}

	pass temp {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_temp();
	}

	pass dof {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_dof();
	}
}
