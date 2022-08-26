
Texture2D shaderTexture;
SamplerState SampleType;

cbuffer MatrixBuffer
{
	matrix gWorldViewProj;
};

struct VertexInputOutput
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};
 
struct PixelInputType {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VertexInputOutput VS(VertexInputOutput input)
{
    VertexInputOutput output; 
    output.position = mul(float4(input.position.xyz, 1.f), gWorldViewProj);
	output.tex = input.tex;
    return output;
}

float4 PS(PixelInputType input) : SV_TARGET
{
        if (input.tex.x < 0.3)
        {
            return float4(1,0,0,1);
        }
    
		float4 textureColor = shaderTexture.Sample(SampleType, input.tex);
		return textureColor;
}

float4 PS1(PixelInputType input) : SV_TARGET
{
        if (input.tex.x > 0.7)
        {
            return float4(1,1,0,1);
        }

		float4 textureColor = shaderTexture.Sample(SampleType, input.tex);
		return textureColor;
}

technique11 RenderTech
{
        pass P1
        {
                SetVertexShader(CompileShader(vs_5_0, VS()));
                SetPixelShader(CompileShader(ps_5_0, PS()));
        }
        pass P0
        {
                SetVertexShader(CompileShader(vs_5_0, VS()));
                SetPixelShader(CompileShader(ps_5_0, PS1()));
        }

}
