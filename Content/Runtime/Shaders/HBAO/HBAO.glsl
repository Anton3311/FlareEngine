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
	float u_TangentBias;
	vec2 u_DepthTextureSize;
	float u_Intensity;

	int u_Debug;
};

layout(set = 3, binding = 0) uniform sampler2D u_NormalTexture;
layout(set = 3, binding = 1) uniform sampler2D u_DepthTexture; // NOTE: Downsampled linear depth

layout(location = 0) in vec2 i_UV;

layout(location = 0) out vec3 o_AO;

const int DIRECTION_COUNT = 8;
const float DIRECTION_ANGLE_STEP = TWO_PI / float(DIRECTION_COUNT);
const int SAMPLE_COUNT = 4;

float Attenuate(float distance)
{
	return max(0.0f, 1.0f - distance / (u_Radius * u_Radius));
}

vec2 SnapToTexelCenter(vec2 uv)
{
	return (floor(uv * u_DepthTextureSize) + 0.5f) / u_DepthTextureSize;
}

float ComputeProjectedSphereSize(float linearDepth)
{
	return u_Radius / linearDepth;
}

vec3 MinByDifference(vec3 position, vec3 left, vec3 right)
{
	vec3 leftDirection = left - position;
	vec3 rightDirection = right - position;

	float leftDistance = dot(leftDirection, leftDirection);
	float rightDistance = dot(rightDirection, rightDirection);

	return (leftDistance < rightDistance) ? left : right;
}

vec3 GetVSPosition(vec2 uv)
{
	float linearDepth = texture(u_DepthTexture, uv).r;
	return ReconstructViewSpacePositionFromDepth(uv * 2.0f - vec2(1.0f),
			LinearDepthToNonLinear(linearDepth));
}

void ComputeDerrivetives(vec3 VSPosition, out vec3 du, out vec3 dv)
{
	vec2 texelSize = vec2(1.0f) / u_DepthTextureSize;

	vec3 left = GetVSPosition(i_UV + vec2(-texelSize.x, 0.0f));
	vec3 right = GetVSPosition(i_UV + vec2(+texelSize.x, 0.0f));

	vec3 top = GetVSPosition(i_UV + vec2(0.0f, +texelSize.y));
	vec3 bottom = GetVSPosition(i_UV + vec2(0.0f, -texelSize.y));

	du = normalize(right - left);
	dv = normalize(top - bottom);
}

#define PER_SAMPLE_AO 1
#define RANDOM_ROTATION 1
#define TANGENT_USING_DU_DV 1
#define DEBUG 0

void main()
{
	vec2 texelSize = vec2(1.0f) / u_DepthTextureSize;
	float depthTextureAspectRatio = u_DepthTextureSize.x / u_DepthTextureSize.y;

	float linearDepth = texture(u_DepthTexture, i_UV).r;
	vec3 viewSpacePosition = ReconstructViewSpacePositionFromDepth(i_UV * 2.0f - vec2(1.0f),
			LinearDepthToNonLinear(linearDepth));

#if TANGENT_USING_DU_DV
	vec3 du, dv;
	ComputeDerrivetives(viewSpacePosition, du, dv);
	dv *= depthTextureAspectRatio;

#if DEBUG
	if (u_Debug == 8)
	{
		o_AO = du;
		return;
	}

	if (u_Debug == 9)
	{
		o_AO = dv;
		return;
	}
#endif
#endif

	vec3 sampledNormal = texture(u_NormalTexture, i_UV).xyz;
	if (all(lessThan(abs(sampledNormal), vec3(0.1f))))
	{
		o_AO = vec3(0.0f);
		return;
	}

	sampledNormal = sampledNormal * 2.0f - vec3(1.0f);

	vec3 viewSpaceNormal = (u_Camera.View * vec4(sampledNormal, 0.0f)).xyz;

	float aoSum = 0.0f;

	float radiusInPixels = ComputeProjectedSphereSize(linearDepth);
	vec2 sampleStep = vec2(radiusInPixels / float(SAMPLE_COUNT));
	sampleStep.x *= depthTextureAspectRatio;

#if DEBUG
	if (u_Debug == 1)
	{
		o_AO = viewSpaceNormal;
		return;
	}

	if (u_Debug == 10)
	{
		o_AO = vec3(sampleStep, 0.0f);
		return;
	}
#endif

#if RANDOM_ROTATION
	float rotationOffset = InterleavedGradientNoise(gl_FragCoord.xy) * TWO_PI;
#else
	float rotationOffset = HALF_PI * u_Intensity;
#endif

	for (int directionIndex = 0; directionIndex < DIRECTION_COUNT; directionIndex++)
	{
		float angle = DIRECTION_ANGLE_STEP * float(directionIndex) + rotationOffset;
		vec2 direction = vec2(cos(angle), sin(angle));
		vec2 uvStep = direction * sampleStep;

#if TANGENT_USING_DU_DV
		vec3 tangentVector = uvStep.x * du + uvStep.y * dv;
#else
		vec3 tangentVector = vec3(direction, 0.0f) - viewSpaceNormal * dot(direction, viewSpaceNormal.xy);
#endif

#if DEBUG
		if (u_Debug == 2)
		{
			o_AO = normalize(tangentVector);
			return;
		}
#endif

		float tangentAngle = atan(tangentVector.z, length(tangentVector.xy)) + u_TangentBias;
		float horizonAngle = tangentAngle;
		float previousAO = 0.0f;

		for (int sampleIndex = 1; sampleIndex <= SAMPLE_COUNT; sampleIndex++)
		{
			vec2 sampleUV = SnapToTexelCenter(i_UV + direction * sampleStep * float(sampleIndex));
			float sampleLinearDepth = texture(u_DepthTexture, sampleUV).r;
			vec3 sampleViewSpacePosition = ReconstructViewSpacePositionFromDepth(
					sampleUV * 2.0f - vec2(1.0f),
					LinearDepthToNonLinear(sampleLinearDepth));

			// D = S_i - P
			vec3 D = sampleViewSpacePosition - viewSpacePosition;

			float elevationAngle = atan(D.z, length(D.xy));

#if DEBUG
			if (u_Debug == 3)
			{
				o_AO = (elevationAngle > tangentAngle) ? vec3(1.0f) : vec3(0.0f);
				return;
			}

			if (u_Debug == 4)
			{
				o_AO = vec3(elevationAngle - tangentAngle);
				return;
			}

			if (u_Debug == 5)
			{
				o_AO = vec3(elevationAngle);
				return;
			}

			if (u_Debug == 6)
			{
				o_AO = vec3(tangentAngle);
				return;
			}

			if (u_Debug == 7)
			{
				o_AO = vec3(sin(elevationAngle) - sin(tangentAngle));
				return;
			}
#endif

#if 0 && DEBUG
			o_AO = vec3(1000.0f);
			return;
#endif

			if (dot(D, D) > u_Radius * u_Radius)
				continue;

#if PER_SAMPLE_AO
			if (elevationAngle > horizonAngle)
			{
				float ao = sin(horizonAngle) - sin(tangentAngle);
				aoSum += (ao - previousAO) * Attenuate(dot(D, D));

				previousAO = ao;
				horizonAngle = elevationAngle;
			}
#else
			horizonAngle = max(horizonAngle, elevationAngle);
#endif
		}


#if !PER_SAMPLE_AO
		float ao = sin(horizonAngle) - sin(tangentAngle);
		aoSum += ao;
#endif
	}

	o_AO = vec3(max(0.0f, 1.0 - aoSum / (TWO_PI) * u_Intensity));
}

#end
