string XFile = "misc\\teapot.x";
int BCLR = 0xff202060;

// transformations
float4x4 WorldViewProjection : WORLDVIEWPROJECTION;
float4x4 WorldView : WORLDVIEW;

// textures
texture EnvironmentMap 
< 
    string SasUiLabel = "Environment Map";
    string SasUiControl= "FilePicker";
    string type = "CUBE"; 
    string name = "light probes\\stpeters_cross.dds"; 
>;

sampler Environment<bool SasUiVisible = false;> = sampler_state
{ 
    Texture = (EnvironmentMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};

struct VS_OUTPUT
{
    float4 pos  : POSITION;
    float3 norm : TEXCOORD0;
    float3 tex  : TEXCOORD1;
};

VS_OUTPUT vertex(
    float3 ipos  : POSITION,
    float3 inorm : NORMAL,
    float3 itex  : TEXCOORD0)
{
    VS_OUTPUT Out;
    Out.pos  = mul(float4(ipos,  1), WorldViewProjection);
    Out.norm = mul(inorm, WorldView);
    Out.tex = reflect(normalize(Out.pos), Out.norm);
    Out.tex = Out.pos;
//    Out.tex = reflect(float3(0, 0, 1), Out.norm);
    return Out;
}


float4 pixel(VS_OUTPUT In) : COLOR
{
    float4 Color;

    In.norm = normalize(In.norm);
    Color = float4(In.norm, 1);

//    float4 Gloss = texCUBE(Environment, In.tex);
    float4 Gloss = texCUBE(Environment, reflect(normalize(In.tex), In.norm));
    Gloss *= 1 + In.norm.z;


return Gloss;

//    Color = 1 - abs(In.norm.z);
/*
    Color *= float4(0.5, 0.5, 0.5, 0.5);
    Color += float4(0.5, 0.5, 0.5, 0.5);
*/
    return Color;
}

technique schvoi
{
    pass P0
    {
        VertexShader = compile vs_2_0 vertex();
        PixelShader  = compile ps_2_0 pixel();
    }
}
