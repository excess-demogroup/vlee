string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

float fade  = 0.75f;
float sobel_fade = 0.5f;

float alpha = 1.f;
float xoffs = 0.f;
float yoffs = 0.f;

float xzoom = 1.f;
float yzoom = 1.f;

float flash = 0.0f;
float fade2 = 1.0f;
float repeat = 3.0f;

// textures
texture tex;
texture color_map;

const float3 skyBottomColor = float3(0.3, 0.4, 0.8) * 1.75;
#define SPHERES 3
float3 spos[SPHERES] = {
   float3(2,0,-0.4),
   float3(-1.3,0,0),
   float3(1,0,2)
}; 

sampler tex_sampler = sampler_state
{
	Texture = (tex);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = WRAP;
	AddressV = WRAP;
};

texture tex2;
sampler tex2_sampler = sampler_state
{
	Texture = (tex2);
	MipFilter = NONE;
	MinFilter = POINT;
	MagFilter = POINT;
	
	AddressU = WRAP;
	AddressV = WRAP;
};

texture invmap;
sampler invmap_sampler = sampler_state
{
	Texture = (invmap);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

texture overlay;
sampler overlay_sampler = sampler_state
{
	Texture = (overlay);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = WRAP;
	AddressV = WRAP;
};


sampler color_map_sampler = sampler_state
{
	Texture = (color_map);
	MipFilter = NONE;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	
	AddressU = CLAMP;
	AddressV = CLAMP;
};

float3 viewPosition : VIEWPOSITION;
float4x4 wvpi : WORLDVIEWPROJECTIONINV;
struct VS_OUTPUT
{
	float4 pos  : POSITION;
	float2 tex  : TEXCOORD1;
	float3 dir  : TEXCOORD2;
	float3 pos2  : TEXCOORD3;
};

VS_OUTPUT vertex(float4 ipos : POSITION, float2 tex  : TEXCOORD0)
{
	VS_OUTPUT Out;
	Out.pos = ipos;
	Out.tex = tex;

	float4 c = mul(ipos, wvpi);
	Out.pos2 = viewPosition;
	Out.dir = (c.xyz / c.w) - viewPosition;

	return Out;
}

float texel_width = 1.f / 800;
float texel_height = 1.f / 450;

float4 sobel(sampler samp, float2 uv)
{
	float4 color1 = float4(0, 0, 0, 0);
	color1 += tex2D(tex_sampler, uv + float2(-texel_width, -texel_height)) * -1;
	color1 += tex2D(tex_sampler, uv + float2(-texel_width, 0)) * -2;
	color1 += tex2D(tex_sampler, uv + float2(-texel_width, +texel_height)) * -1;
	color1 += tex2D(tex_sampler, uv + float2(+texel_width, -texel_height)) *  1;
	color1 += tex2D(tex_sampler, uv + float2(+texel_width, 0)) *  2;
	color1 += tex2D(tex_sampler, uv + float2(+texel_width, +texel_height)) * 1;

	float4 color2 = float4(0, 0, 0, 0);
	color2 += tex2D(tex_sampler, uv + float2(-texel_width, -texel_height)) * -1;
	color2 += tex2D(tex_sampler, uv + float2(0,            -texel_height)) * -2;
	color2 += tex2D(tex_sampler, uv + float2(+texel_width, -texel_height)) * -1;
	color2 += tex2D(tex_sampler, uv + float2(-texel_width, +texel_height)) *  1;
	color2 += tex2D(tex_sampler, uv + float2(0,            +texel_height)) *  2;
	color2 += tex2D(tex_sampler, uv + float2(+texel_width, +texel_height)) * 1;
	
	float val = sqrt(color1.x * color1.x + color2.x * color2.x);
	return float4(val, val, val, val);
}

float luminance(float3 color)
{
	return
		(color.r * 0.299) +
		(color.g * 0.587) +
		(color.b * 0.114);
}


float3 environment(float3 dir)
{
   float3 bg = lerp(
      skyBottomColor,
      float3(0.3, 0.35, 0.8) * 0.8,
      pow(max(0, dir.y), 0.6));
   return bg + pow(max(0, dot(dir, normalize(float3(1,2,0)))), 100) * float3(2.5, 3, 0.0) * 0.5;
}

float traceSphere(float3 pos, float3 dir, float3 spos, float rr)
{
   float3 dst = pos - spos;
   float a = dot(dst, dst);
   float b = dot(dst, dir);
   float c = a - rr;
   float d = b * b - c;

   if (d > 0)
      return -b - sqrt(d);
   else
      return -1;
}

float4 trace(float3 pos, float3 dir)
{
   float mint = 999999;
   int sphereHit = -1;
   for (int i = 0; i < SPHERES; ++i) {
      float t = traceSphere(pos, dir, spos[i], 1);
      if (t > 0 && t < mint) {
         sphereHit = i;
         mint = t;
      }
   }

    float totalt = 0;
   if (sphereHit >= 0) {
      pos += dir * mint;
      totalt += mint;
      float3 n = pos - spos[sphereHit];
      dir = reflect(dir, n);

      for (int i = 0; i < 4; ++i) {
         mint = 999999;
         int currSphere = sphereHit;
         sphereHit = -1;
         for (int i = 0; i < SPHERES; ++i) {
            if (i == currSphere)
               continue;
            float t = traceSphere(pos, dir, spos[i], 1);
            if (t > 0 && t < mint) {
               sphereHit = i;
               mint = t;
            }
         }
   
         if (sphereHit < 0)
            break;

         pos += dir * mint;
         totalt += mint;
         float3 n = pos - spos[sphereHit];
         dir = reflect(dir, n);
      }
   }

   float dist = 1.0;
   float t = -(pos.y + dist) / dir.y;
   float2 fw = fwidth(pos.xz + dir.xz * t);
 
   if (dir.y < 0 && t > 0) {
      pos += dir * t;
         totalt += t;

      float2 fuzz = fw * 2;
      float fuzzMax = max(fw.x, fw.y);
      float2 checkPos = frac(pos.xz + fuzz * 0.5);
      float2 pp = 
         smoothstep(float2(0.5, 0.5), float2(0.5, 0.5) + fuzz, checkPos) +
         (1.0 - smoothstep(float2(0,0), fuzz, checkPos));
      float p = pp.x * pp.y + (1.0 - pp.x) * (1.0 - pp.y);
      p = lerp(p, 0.5, smoothstep(0.125, 0.5, fuzzMax));

      float3 n = float3(0, 1, 0);
      float ao = 1.0;
      for (int i = 0; i < SPHERES; ++i) {
          ao += min(0, normalize(pos - spos[i]).y) *
                pow(1 / distance(pos, spos[i]), 2);
      }
//      float3 c = float3(p,p,p);
     p *= ao;
     float3 c = lerp(skyBottomColor, float3(p,p,p), 1 / max(1, 0.75 + totalt * 0.04));
      return float4(c,1);
   } else 
      return float4(environment(dir), 1);
}

float2 bloom_nudge = float2(0.5 / 400, 0.5 / 225);
bool spheretracer;


float4 pixel(VS_OUTPUT In) : COLOR
{
	float pal_sel = 0.0;

	float4 color =
		tex2D(tex_sampler, In.tex * repeat + bloom_nudge) * 0.25
		+ tex2D(tex2_sampler, In.tex * repeat) * 1.0 ;

	if (spheretracer)
		color = trace(In.pos2, normalize(In.dir));

//	color.rgb = lerp(color.rgb, tex2D(tex_sampler, In.tex + bloom_nudge).rgb, (1 - pal_sel) * 0.75);
	
	/* lookup in palette */
	float lum = luminance(color.rgb);
//	float lum = (color.r + color.g + color.b) / 3;
	
//	float pal_sel = 1 - length(In.tex - 0.5) * 1.5;
	
	float3 pal_color = tex2D(color_map_sampler, float2(lum, 1.0 * pal_sel)).rgb;
	color.rgb = lerp(color.rgb, pal_color, fade);
	
	
//	float3 delta = color - lum;
//	color.rgb += delta * (1-pal_sel);
	color = color * fade2 + flash;
	color.rgb *= tex2D(overlay_sampler, In.tex * repeat).r;
	color.rgb = lerp(color.rgb, float3(1,1,1) - color.rgb, tex2D(invmap_sampler, In.tex).r);
	
	return color;
}

technique blur_ps_vs_2_0
{
	pass P0
	{
		VertexShader = compile vs_3_0 vertex();
		PixelShader  = compile ps_3_0 pixel();
	}
}
