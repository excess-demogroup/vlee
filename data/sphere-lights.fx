float4x4 matWorldView : WORLDVIEW;
float4x4 matWorldViewProjection : WORLDVIEWPROJECTION;
float4x4 matWorldViewInverse : WORLDVIEWINVERSE;
float scroll;

texture mask_tex;
sampler mask_samp = sampler_state {
	Texture = (mask_tex);
	MipFilter = LINEAR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
	sRGBTexture = TRUE;
};

texture intensity_tex;
sampler intensity_samp = sampler_state {
	Texture = (intensity_tex);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	AddressU = CLAMP;
	AddressV = CLAMP;
	sRGBTexture = TRUE;
};

struct VS_INPUT {
	float4 Position : POSITION;
	float3 Normal : NORMAL;
	float2 Uv : TEXCOORD0;
};

struct VS_OUTPUT {
	float4 Position : POSITION;
	float3 Normal : TEXCOORD0;
	float4 Pos2 : TEXCOORD1;
	float2 Uv : TEXCOORD2;
	float2 Uv2 : TEXCOORD3;
};

VS_OUTPUT vs_main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	float3 pos = Input.Position.xyz;

	pos = normalize(float3(pos.xz, -32)) * 128.0;
	pos.z += 128;

	Output.Position = mul( float4(pos, 1), matWorldViewProjection );
	Output.Normal = mul(matWorldViewInverse, float4(Input.Normal, 0)).xyz;
	Output.Pos2 = mul(float4(pos, 1), matWorldView);
	Output.Uv = Input.Uv;

	//anim:
	Output.Uv.x += scroll;

	Output.Uv2 = Input.Uv; // + frac(floor(float2(sin(time * 33), 
							//                 cos(time * 32)) * 100) / 128.0);
	return Output;
}

struct PS_OUTPUT {
	float4 col : COLOR0;
	float4 z : COLOR1;
};

PS_OUTPUT ps_main(VS_OUTPUT Input)
{
   PS_OUTPUT o;
   float light_intensity = 0.0025 + tex2D(intensity_samp, Input.Uv).r * 4;
   float light_mask = tex2D(mask_samp, Input.Uv * 128.0).r;
   // float3 light_color = float3(0.5,1,1); // tex2D(Texture2, Input.Uv2);
   float3 light_color = float3(1,0.5,1); // tex2D(Texture2, Input.Uv2);
//   light_color = normalize(desaturate(light_color, 0.75)) * 2;
      
//   light_color = pow(light_color, 5) * 3.5;
   
   o.col = float4((light_intensity * light_mask) * light_color, 1);
   o.z = Input.Pos2.z;

   return o;
}

technique cube_room {
	pass P0 {
		VertexShader = compile vs_2_0 vs_main();
		PixelShader  = compile ps_2_0 ps_main();
	}
}
