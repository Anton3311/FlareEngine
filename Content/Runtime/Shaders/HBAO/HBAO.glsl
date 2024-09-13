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
};

layout(set = 3, binding = 0) uniform sampler2D u_NormalTexture;
layout(set = 3, binding = 1) uniform sampler2D u_DepthTexture; // NOTE: Downsampled linear depth

layout(location = 0) in vec2 i_UV;

layout(location = 0) out float o_AO;

const int DIRECTION_COUNT = 8;
const float DIRECTION_ANGLE_STEP = TWO_PI / float(DIRECTION_COUNT);
const int SAMPLE_COUNT = 16;

float Attenuate(float distance)
{
	return max(0.0f, 1.0f - distance / u_Radius);
}

vec2 SnapToTexelCenter(vec2 uv)
{
	return (round(uv * u_DepthTextureSize) + 0.5f) / u_DepthTextureSize;
}

float ComputeProjectedSphereSize(float linearDepth)
{
	// NOTE: -linearDetph because camera's view direction in view space is -Z axis

	vec4 clipSpace = vec4(u_Radius, 0.0f, -linearDepth, 1.0f);
	vec4 projected = u_Camera.Projection * clipSpace;
	
	return min(1.0f, projected.x / projected.w);
}

void main()
{
	float linearDepth = texture(u_DepthTexture, i_UV).r;
	vec3 viewSpacePosition = ReconstructViewSpacePositionFromDepth(i_UV * 2.0f - vec2(1.0f),
			LinearDepthToNonLinear(linearDepth));

	vec3 sampledNormal = texture(u_NormalTexture, i_UV).xyz;
	if (all(lessThan(abs(sampledNormal), vec3(0.1f))))
	{
		o_AO = 0.0f;
		return;
	}

	sampledNormal = sampledNormal * 2.0f - vec3(1.0f);

	vec3 viewSpaceNormal = (u_Camera.View * vec4(sampledNormal, 0.0f)).xyz;
	vec2 texelSize = vec2(1.0f) / u_DepthTextureSize;

	float aoSum = 0.0f;

	float radiusInPixels = ComputeProjectedSphereSize(linearDepth);
	vec2 sampleStep = vec2(radiusInPixels / float(SAMPLE_COUNT));
	sampleStep.x /= u_DepthTextureSize.x / u_DepthTextureSize.y;

	float rotationOffset = InterleavedGradientNoise(gl_FragCoord.xy) * TWO_PI;

	for (int directionIndex = 0; directionIndex < DIRECTION_COUNT; directionIndex++)
	{
		float angle = DIRECTION_ANGLE_STEP * float(directionIndex) + rotationOffset;
		vec2 direction = vec2(cos(angle), sin(angle));

		vec3 tangentVector = vec3(direction, 0.0f) - viewSpaceNormal * dot(direction, viewSpaceNormal.xy);
		tangentVector = normalize(tangentVector);

		float tangentAngle = atan(tangentVector.z, length(tangentVector.xy)) + u_TangentBias;
		float horizonAngle = tangentAngle;

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

			horizonAngle = max(horizonAngle, elevationAngle);
		}

		float ao = sin(horizonAngle) - sin(tangentAngle);
		aoSum += ao;// * Attenuate(distance(viewSpacePosition, D));
	}

	o_AO = 1.0 - aoSum / (TWO_PI);
}

#end
