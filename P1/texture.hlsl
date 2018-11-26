Texture2D texture1;
SamplerState samplerState;

cbuffer cbPerObject
{
	float4x4 worldViewProjectionMatrix;
};



struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float2 textureCoordinate : TEXCOORD;
	float3 normal : NORMAL;
};

VS_OUTPUT vertexShaderMain(float3 position : POSITION, float2 textureCoordinate : TEXCOORD, float3 normal : NORMAL)
{
	VS_OUTPUT output;

	output.position = mul(float4(position, 1), worldViewProjectionMatrix);
	output.textureCoordinate = textureCoordinate;
	output.normal = normal;

    return output;
}

float4 pixelShaderMain(VS_OUTPUT input) : SV_TARGET
{
	float4 sampledColor = texture1.Sample(samplerState, input.textureCoordinate);

    return sampledColor;
}