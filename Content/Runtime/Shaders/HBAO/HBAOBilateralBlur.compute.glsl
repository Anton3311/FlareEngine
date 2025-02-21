#begin compute
#version 450

const uint TILE_SIZE = 16;

layout(set = 3, binding = 0, r8) uniform readonly image2D u_AO;
layout(set = 3, binding = 1, r8) uniform writeonly image2D u_OutputAO;
layout(set = 3, binding = 2, r32f) uniform readonly image2D u_LinearDepth;

const int KERNEL_RADIUS = 3;

layout(std140, push_constant) uniform Constants
{
	ivec2 u_Direction;
	ivec2 u_ImageSize;
	float u_Sharpness;
};

float ComputeBlur(ivec2 texelPosition, float distance, float centerDepth, float centerValue, inout float totalWeight)
{
	float linearDepth = imageLoad(u_LinearDepth, texelPosition).r;
	float ao = imageLoad(u_AO, texelPosition).r;

	float depthDifference = (linearDepth - centerDepth) * u_Sharpness;

	float blurSigma = float(KERNEL_RADIUS) * 0.5f;
	float blurFalloff = 2.0f * (blurSigma * blurSigma);

	float spacialWeight = (distance * distance) / blurFalloff;
	
	float weight = exp(-spacialWeight - depthDifference * depthDifference);

	totalWeight += weight;
	return ao * weight;
}

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;
void main()
{
	ivec2 texelPosition = ivec2(gl_GlobalInvocationID.xy);
	if (texelPosition.x >= u_ImageSize.x || texelPosition.y >= u_ImageSize.y)
		return;

	float centerDepth = imageLoad(u_LinearDepth, texelPosition).r;
	float centerAO = imageLoad(u_AO, texelPosition).r;

	// Avoid computing blur for the center pixel, just initialize with precomputed values
	float totalAO = centerAO;
	float totalWeight = 1.0f;

	for (int i = 1; i < KERNEL_RADIUS; i++)
	{
		ivec2 sampleTexelPosition = texelPosition + u_Direction * i;
		totalAO += ComputeBlur(sampleTexelPosition, float(i), centerDepth, centerAO, totalWeight);
	}

	for (int i = 1; i < KERNEL_RADIUS; i++)
	{
		ivec2 sampleTexelPosition = texelPosition - u_Direction * i;
		totalAO += ComputeBlur(sampleTexelPosition, float(i), centerDepth, centerAO, totalWeight);
	}

	float ao = totalAO / totalWeight;
	imageStore(u_OutputAO, texelPosition, vec4(ao, ao, ao, 1.0f));
}

#end
