#begin vertex
#version 450

layout(location = 0) in vec3 i_Position;

layout(location = 0) out vec2 o_UV;

void main()
{
	gl_Position = vec4(i_Position, 1.0f);

	o_UV = i_Position.xy * 0.5f + vec2(0.5f);
}

#end

#begin pixel
#version 450

layout(location = 0) in vec2 i_UV;

layout(set = 3, binding = 0) uniform sampler2D u_AO;
layout(set = 3, binding = 1) uniform sampler2D u_LinearDepth;

layout(location = 0) out float o_Result;

const float KERNEL_RADIUS = 3;

layout(std140, push_constant) uniform Constants
{
	float u_Sharpness;
	vec2 u_Direction;
};

float ComputeBlur(vec2 uv, float distance, float centerDepth, float centerValue, inout float totalWeight)
{
	float linearDepth = texture(u_LinearDepth, uv).r;
	float ao = texture(u_AO, uv).r;

	float depthDifference = (linearDepth - centerDepth) * u_Sharpness;

	float blurSigma = KERNEL_RADIUS * 0.5f;
	float blurFalloff = 2.0f * (blurSigma * blurSigma);

	float spacialWeight = (distance * distance) / blurFalloff;
	
	float weight = exp(-spacialWeight - depthDifference * depthDifference);

	totalWeight += weight;
	return ao * weight;

}

void main()
{
	float centerDepth = texture(u_LinearDepth, i_UV).r;
	float centerAO = texture(u_AO, i_UV).r;

	// Avoid computing blur for the center pixel, just initialize with precomputed values
	float totalAO = centerAO;
	float totalWeight = 1.0f;

	for (float i = 1; i < KERNEL_RADIUS; i++)
	{
		vec2 uv = i_UV + u_Direction * i;
		totalAO += ComputeBlur(uv, i, centerDepth, centerAO, totalWeight);
	}

	for (float i = 1; i < KERNEL_RADIUS; i++)
	{
		vec2 uv = i_UV - u_Direction * i;
		totalAO += ComputeBlur(uv, i, centerDepth, centerAO, totalWeight);
	}

	o_Result = totalAO / totalWeight;
}

#end
