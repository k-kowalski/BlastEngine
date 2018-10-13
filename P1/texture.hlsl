Texture2D texture1;
SamplerState ObjSamplerState;

cbuffer cbPerObject
{
	float4x4 worldViewProjectionMatrix;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float2 textureCoordinate : TEXCOORD;
};

VS_OUTPUT vertexShaderMain(float3 position : position, float2 textureCoordinate : textureCoordinate)
{
	VS_OUTPUT output;

	output.position = mul(float4(position.xyz, 1), worldViewProjectionMatrix);

	output.textureCoordinate = textureCoordinate;

    return output;
}

float4 pixelShaderMain(VS_OUTPUT input) : SV_TARGET
{
    return texture1.Sample( ObjSamplerState, input.textureCoordinate );
}