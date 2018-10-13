Texture2D ObjTexture;
SamplerState ObjSamplerState;

cbuffer cbPerObject
{
	float4x4 worldViewProjectionMatrix;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VS_OUTPUT vertexShaderMain(float4 position : position, float4 color : color)
{
	VS_OUTPUT output;
	output.position = mul(position, worldViewProjectionMatrix);
	output.color = color;

    return output;
}

float4 pixelShaderMain(VS_OUTPUT input) : SV_TARGET
{
    return input.color;
}