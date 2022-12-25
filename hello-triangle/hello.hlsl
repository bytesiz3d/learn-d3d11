/// Vertex

struct VS_Input
{
    float3 position: POSITION;
    float3 color: COLOR0;
};

struct VS_Output
{
    float4 position: SV_Position;
    float3 color: COLOR0;
};

VS_Output VS_Main(VS_Input input)
{
    VS_Output output = (VS_Output)0;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}

/// Pixel

struct PS_Input
{
    float4 position: SV_Position;
    float3 color: COLOR0;
};

struct PS_Output
{
    float4 color: SV_Target0;
};

PS_Output PS_Main(PS_Input input)
{
    PS_Output output = (PS_Output)0;
    output.color = float4(input.color, 1.0);
    return output;
}