#begin compute
#version 450

#include "../Common/Camera.glsl"
#include "../Common/Math.glsl"

const uint TILE_SIZE = 16;
const int DIRECTION_COUNT = 8;
const float DIRECTION_ANGLE_STEP = TWO_PI / float(DIRECTION_COUNT);
const int SAMPLE_COUNT = 4;

layout(set = 3, binding = 0) uniform sampler2D u_DepthTexture; // NOTE: Linear depth
layout(set = 3, binding = 1, rgba32f) uniform writeonly image2D u_OutputAO;

layout(push_constant) uniform Constants
{
	float u_Radius;
	float u_RadiusSquared;
	float u_NegativeInverseSquaredRadius;
	float u_Bias;
	float u_Intensity;

	vec2 u_DepthTextureSize;
	vec2 u_InverseDepthTextureSize;

	vec2 u_InverseProjectionParams;

	ivec2 u_SampleOffset;
	float u_JitterAngle;

	ivec2 u_OutputImageSize;
};

vec3 MinDifference(vec3 position, vec3 left, vec3 right)
{
	vec3 leftDirection = position - left;
	vec3 rightDirection = right - position;

	float leftDistance = dot(leftDirection, leftDirection);
	float rightDistance = dot(rightDirection, rightDirection);

	return (leftDistance < rightDistance) ? leftDirection : rightDirection;
}

vec3 FetchPositionVS(ivec2 texelPosition)
{
	float linearDepth = texelFetch(u_DepthTexture, texelPosition, 0).r;
	vec2 uv = vec2(texelPosition) * u_InverseDepthTextureSize;
	uv = uv * 2.0f - vec2(1.0f);

	// -linearDepth because -Z is forward
	return vec3(uv * u_InverseProjectionParams * linearDepth, -linearDepth);
}

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;
void main()
{
	if (gl_GlobalInvocationID.x >= u_OutputImageSize.x || gl_GlobalInvocationID.y >= u_OutputImageSize.y)
	{
		return;
	}

	ivec2 texelPosition = ivec2(gl_GlobalInvocationID.xy) * 2 + u_SampleOffset;
	vec2 uv = vec2(texelPosition) * u_InverseDepthTextureSize;

	vec3 positionVS = FetchPositionVS(texelPosition);

	vec3 left = FetchPositionVS(texelPosition - ivec2(1, 0));
	vec3 right = FetchPositionVS(texelPosition + ivec2(1, 0));
	vec3 top = FetchPositionVS(texelPosition + ivec2(0, 1));
	vec3 bottom = FetchPositionVS(texelPosition - ivec2(0, 1));

	vec3 du = MinDifference(positionVS, left, right);
	vec3 dv = MinDifference(positionVS, bottom, top);

	vec3 normalVS = normalize(cross(du, dv));

#if 1
	imageStore(u_OutputAO, ivec2(gl_GlobalInvocationID.xy), vec4(normalVS, 1.0f));
	return;
#endif
}

#end
