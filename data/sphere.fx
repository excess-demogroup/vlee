float4x4 matView : VIEW;
float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;
float time1, time2;
float freq1, freq2;
float amt1, amt2;

textureCUBE env_tex;
samplerCUBE env_samp = sampler_state {
	Texture = (env_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	sRGBTexture = TRUE;
};

struct VS_INPUT {
	float4 Position : POSITION0;
	float3 Normal : NORMAL;
};

struct VS_OUTPUT {
	float4 Position : POSITION0;
	float3 Normal : TEXCOORD0;
	float3 Pos2 : TEXCOORD2;
};

VS_OUTPUT vs_main( VS_INPUT Input )
{
	VS_OUTPUT Output;

	float3 pos = Input.Position;
	float3 opos = pos;
	pos += opos * pow(length(cos(opos * freq1 + time1)), 3) * amt1;
	pos += opos * pow(length(cos(opos * freq2 + time2)), 3) * amt2;
	pos *= 3;

	Output.Position = mul( float4(pos, 1), matWorldViewProjection );
	Output.Normal = mul( matWorldViewInverse, Input.Normal );
	Output.Pos2 = mul( float4(pos, 1), matWorldView );
	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
	float3 n = normalize(cross(ddx(Input.Pos2), ddy(Input.Pos2)));
//  n = Input.Normal;

	PS_OUTPUT o;
	o.col = float4(texCUBE(env_samp, reflect(n, float3(0,0,1))).rgb * 3, 1);
	o.col = pow(o.col, 1.25);
	o.col.rgb = o.col.bgr;
	// o.col.rgb = lerp(o.col.rgb, o.col.ggg, 0.8);
	o.z = Input.Pos2.z;
	return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_3_0 vs_main();
		PixelShader  = compile ps_3_0 ps_main();
	}
}
