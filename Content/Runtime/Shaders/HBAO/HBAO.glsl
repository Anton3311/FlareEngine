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

#include "../Common/Camera.glsl"
#include "../Common/Math.glsl"

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
};

layout(set = 3, binding = 0) uniform sampler2D u_DepthTexture; // NOTE: Downsampled linear depth

layout(location = 0) in vec2 i_UV;

layout(location = 0) out vec3 o_AO;

const int DIRECTION_COUNT = 8;
const float DIRECTION_ANGLE_STEP = TWO_PI / float(DIRECTION_COUNT);
const int SAMPLE_COUNT = 4;

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
	float linearDepth = texture(u_DepthTexture, uv + u_SampleOffset * u_InverseDepthTextureSize).r;

	uv = uv * 2.0f - vec2(1.0f);

	// -linearDepth because -Z is forward
	return vec3(uv * u_InverseProjectionParams * linearDepth, -linearDepth);
}

void ComputeDerrivatives(vec3 VSPosition, out vec3 du, out vec3 dv)
{
	vec2 texelSize = u_InverseDepthTextureSize;

	vec3 left = GetVSPosition(i_UV + vec2(-texelSize.x, 0.0f));
	vec3 right = GetVSPosition(i_UV + vec2(+texelSize.x, 0.0f));

	vec3 top = GetVSPosition(i_UV + vec2(0.0f, +texelSize.y));
	vec3 bottom = GetVSPosition(i_UV + vec2(0.0f, -texelSize.y));

	du = MinDifference(VSPosition, left, right);
	dv = MinDifference(VSPosition, bottom, top);
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

float ComputeAO(vec3 positionVS, vec3 normalVS, float tangentAngle, vec2 sampleStep)
{
	float horizonAngle = tangentAngle;
	float previousAO = 0.0f;

	float totalAO = 0.0f;

	for (int sampleIndex = 1; sampleIndex <= SAMPLE_COUNT; sampleIndex++)
	{
		vec2 sampleUV = SnapToTexelCenter(i_UV + sampleStep * float(sampleIndex));
		vec3 sampleViewSpacePosition = GetVSPosition(sampleUV);

#if 0
		// D = S_i - P
		vec3 D = sampleViewSpacePosition - positionVS;

		float elevationAngle = atan(D.z, length(D.xy));

		float lengthSqaured = dot(D, D);
		if (lengthSqaured > u_RadiusSquared)
			continue;

		if (elevationAngle > horizonAngle)
		{
			float ao = sin(elevationAngle) - sin(tangentAngle);
			totalAO += (ao - previousAO) * Attenuate(lengthSqaured);

			previousAO = ao;
			horizonAngle = elevationAngle;
		}
#else
		totalAO += ComputeSampleAO(positionVS, normalVS, sampleViewSpacePosition);
#endif
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

void main()
{
	vec3 positionVS = GetVSPosition(i_UV);

	vec3 du, dv;
	ComputeDerrivatives(positionVS, du, dv);

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

		aoSum += ComputeAO(positionVS, normalVS, tangentAngle, direction * sampleStep);
	}

	o_AO = vec3(max(0.0f, 1.0 - aoSum / (TWO_PI) * u_Intensity));
}

#end
