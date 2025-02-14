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

float Attenuate(float distanceSquared)
{
	return max(0.0f, 1.0f + distanceSquared * u_NegativeInverseSquaredRadius);
}

vec2 SnapToTexelCenter(vec2 uv)
{
	return (floor(uv * u_DepthTextureSize) + 0.5f) * u_InverseDepthTextureSize;
}

vec3 MinDifference(vec3 position, vec3 left, vec3 right)
{
	vec3 leftDirection = position - left;
	vec3 rightDirection = right - position;

	float leftDistance = dot(leftDirection, leftDirection);
	float rightDistance = dot(rightDirection, rightDirection);

	return (leftDistance < rightDistance) ? leftDirection : rightDirection;
}

vec3 GetVSPosition(vec2 uv)
{
	float linearDepth = texture(u_DepthTexture, uv).r;
	uv = uv * 2.0f - vec2(1.0f);
	// -linearDepth because -Z is forward
	return vec3(uv * u_InverseProjectionParams * linearDepth, -linearDepth);
}

vec3 FetchPositionVS(ivec2 texelPosition)
{
	float linearDepth = texelFetch(u_DepthTexture, texelPosition, 0).r;
	vec2 uv = vec2(texelPosition) * u_InverseDepthTextureSize;
	uv = uv * 2.0f - vec2(1.0f);

	// -linearDepth because -Z is forward
	return vec3(uv * u_InverseProjectionParams * linearDepth, -linearDepth);
}

float ComputeScreenSpaceRadius(vec3 positionVS)
{
	vec4 projectedCenter = u_Camera.Projection * vec4(positionVS, 1.0f);
	projectedCenter /= projectedCenter.w;

	vec4 projectedPoint = u_Camera.Projection * vec4(positionVS + vec3(u_Radius, 0.0f, 0.0f), 1.0f);
	projectedPoint /= projectedPoint.w;

	return (projectedPoint.x - projectedCenter.x) * 0.5f;
}

float ComputeSampleAO(vec3 positionVS, vec3 normalVS, vec3 samplePositionVS)
{
	vec3 D = samplePositionVS - positionVS;
	float DdotD = dot(D, D);
	float NdotD = dot(normalVS, D) * inversesqrt(max(DdotD, 0.001f)); // max(0.001f) to avoid divison by zero

	return clamp(NdotD - u_Bias, 0.0f, 1.0f) * clamp(Attenuate(DdotD), 0.0f, 1.0f);
}

float ComputeAO(vec2 uv, vec3 positionVS, vec3 normalVS, float tangentAngle, vec2 sampleStep)
{
	float horizonAngle = tangentAngle;
	float previousAO = 0.0f;

	float totalAO = 0.0f;

	for (int sampleIndex = 1; sampleIndex <= SAMPLE_COUNT; sampleIndex++)
	{
		vec2 sampleUV = SnapToTexelCenter(uv + sampleStep * float(sampleIndex));
		vec3 sampleViewSpacePosition = GetVSPosition(sampleUV);

		totalAO += ComputeSampleAO(positionVS, normalVS, sampleViewSpacePosition);
	}

	return totalAO;
}

vec2 ComputeSampleStep(vec3 positionVS)
{
	float depthTextureAspectRatio = u_DepthTextureSize.x / u_DepthTextureSize.y;

	float radiusInPixels = ComputeScreenSpaceRadius(positionVS);
	radiusInPixels /= 4.0f;

	vec2 sampleStep = vec2(radiusInPixels / float(SAMPLE_COUNT + 1));
	sampleStep.x /= depthTextureAspectRatio;

	return sampleStep;
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

	vec2 sampleStep = ComputeSampleStep(positionVS);

	float aoSum = 0.0f;
	float rotationOffset = u_JitterAngle;

	for (int directionIndex = 0; directionIndex < DIRECTION_COUNT; directionIndex++)
	{
		float angle = DIRECTION_ANGLE_STEP * float(directionIndex) + rotationOffset;
		vec2 direction = vec2(cos(angle), sin(angle));

		vec3 tangentVector = direction.x * du + direction.y * dv;
		float tangentAngle = atan(tangentVector.z, length(tangentVector.xy)) + u_Bias;

		aoSum += ComputeAO(uv, positionVS, normalVS, tangentAngle, direction * sampleStep);
	}

	float result = max(0.0f, 1.0 - aoSum / (TWO_PI) * u_Intensity);

	imageStore(u_OutputAO, ivec2(gl_GlobalInvocationID.xy), vec4(vec3(result), 1.0f));
}

#end
