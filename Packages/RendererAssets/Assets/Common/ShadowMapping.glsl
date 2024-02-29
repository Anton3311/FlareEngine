#ifndef SHADOW_MAPPING_H
#define SHADOW_MAPPING_H

#include "Light.glsl"
#include "Math.glsl"

const int CASCADES_COUNT = 4;

layout(std140, binding = 2) uniform ShadowData
{
	float u_Bias;
	float u_LightFrustumSize;
	float u_LightSize;

	int u_MaxCascadeIndex;

	vec4 u_CascadeSplits;

	mat4 u_CascadeProjection0;
	mat4 u_CascadeProjection1;
	mat4 u_CascadeProjection2;
	mat4 u_CascadeProjection3;

	float u_ShadowResolution;
	float u_ShadowSoftness;
	float u_ShadowFadeDistance;
	float u_MaxShadowDistance;
};

layout(binding = 28) uniform sampler2D u_ShadowMap0;
layout(binding = 29) uniform sampler2D u_ShadowMap1;
layout(binding = 30) uniform sampler2D u_ShadowMap2;
layout(binding = 31) uniform sampler2D u_ShadowMap3;

// Vogel disk points
const vec2[] SAMPLE_POINTS = {
	vec2(0.1767766952966369, 0.0),
	vec2(-0.22577979282638214, 0.20681751654846825),
	vec2(0.03458701007725054, -0.3937686360464939),
	vec2(0.28453027372165623, 0.3712041531770346),
	vec2(-0.5222095951380513, -0.09245073685889445),
	vec2(0.4947532383785339, -0.3145937588604606),
	vec2(-0.16560172116300248, 0.6154884807596737),
	vec2(-0.3154050676060234, -0.6076756069881691),
	vec2(0.6845685825311724, 0.2502316043413811),
	vec2(-0.7123533443287733, 0.29377323367456754),
	vec2(0.34362426953239034, -0.7336023182817316),
	vec2(0.2534030290286406, 0.8090345511034185),
	vec2(-0.7645502647498922, -0.4435232718481295),
	vec2(0.8972281608231768, -0.19680352493250616),
	vec2(-0.5479077316191127, 0.778490281013192),
	vec2(-0.1259483874642235, -0.9761593126611875),
};

const int NUMBER_OF_SAMPLES = 16;
#define LIGHT_SIZE (u_LightSize / u_LightFrustumSize)

float CalculateBlockerDistance(sampler2D shadowMap, vec3 projectedLightSpacePosition, vec2 rotation, float bias, float scale)
{
	float receieverDepth = projectedLightSpacePosition.z;
	float blockerDistance = 0.0;
	float samplesCount = 0;
	float searchSize = LIGHT_SIZE * (receieverDepth - u_LightNear) / receieverDepth * scale;

	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		vec2 offset = vec2(
			rotation.x * SAMPLE_POINTS[i].x - rotation.y * SAMPLE_POINTS[i].y,
			rotation.y * SAMPLE_POINTS[i].x + rotation.x * SAMPLE_POINTS[i].y
		);

		float depth = texture(shadowMap, projectedLightSpacePosition.xy + offset * searchSize).r;
		if (depth < receieverDepth - bias)
		{
			samplesCount += 1.0;
			blockerDistance += depth;
		}
	}

	if (samplesCount == 0.0)
		return -1.0;
	
	return max(0.01, blockerDistance / samplesCount);
}

float PCF(sampler2D shadowMap, vec2 uv, float receieverDepth, float filterRadius, vec2 rotation, float bias)
{
	float shadow = 0.0f;
	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		vec2 offset = vec2(
			rotation.x * SAMPLE_POINTS[i].x - rotation.y * SAMPLE_POINTS[i].y,
			rotation.y * SAMPLE_POINTS[i].x + rotation.x * SAMPLE_POINTS[i].y
		);

		float sampledDepth = texture(shadowMap, uv + offset * filterRadius).r;
		shadow += (receieverDepth - bias > sampledDepth ? 1.0 : 0.0);
	}
	
	return shadow / NUMBER_OF_SAMPLES;
}

float CalculateCascadeShadow(sampler2D shadowMap, vec4 lightSpacePosition, float bias, float poissonPointsRotationAngle, float scale)
{
	vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
	projected = projected * 0.5 + vec3(0.5);

	vec2 uv = projected.xy;
	float receieverDepth = projected.z;

	if (receieverDepth > 1.0)
		return 1.0;

	if (projected.x > 1.0 || projected.y > 1.0 || projected.x < 0 || projected.y < 0)
		return 1.0;

	vec2 rotation = vec2(cos(poissonPointsRotationAngle), sin(poissonPointsRotationAngle));

	float blockerDistance = CalculateBlockerDistance(shadowMap, projected, rotation, bias, scale);
	if (blockerDistance == -1.0f)
		return 1.0f;

	float penumbraWidth = (receieverDepth - blockerDistance) / blockerDistance;
	float filterRadius = penumbraWidth * LIGHT_SIZE * u_LightNear / receieverDepth;
	filterRadius = max(0.0f, filterRadius * scale * u_ShadowSoftness);

	return 1.0f - PCF(shadowMap, uv, receieverDepth, filterRadius, rotation, bias);
}

float CalculateShadow(vec3 N, vec4 position, vec3 viewSpacePosition)
{
	float viewSpaceDistance = abs(viewSpacePosition.z);

	int cascadeIndex = CASCADES_COUNT;
	for (int i = 0; i < CASCADES_COUNT; i++)
	{
		if (viewSpaceDistance <= u_CascadeSplits[i])
		{
			cascadeIndex = i;
			break;
		}
	}

	float shadowFade = smoothstep(u_ShadowFadeDistance, u_MaxShadowDistance, viewSpaceDistance);

	float shadow = 0.0f;
	float NoL = dot(N, -u_LightDirection);
	float bias = max(u_Bias * (1.0f - NoL), 0.0f);

	bias /= float(cascadeIndex + 1);

	float rotationAngle = 2.0f * PI * InterleavedGradientNoise(gl_FragCoord.xy);

	switch (cascadeIndex)
	{
	case 0:
		shadow = CalculateCascadeShadow(u_ShadowMap0, (u_CascadeProjection0 * position), bias, rotationAngle, 1.0f);
		break;
	case 1:
		shadow = CalculateCascadeShadow(u_ShadowMap1, (u_CascadeProjection1 * position), bias, rotationAngle, 0.5f);
		break;
	case 2:
		shadow = CalculateCascadeShadow(u_ShadowMap2, (u_CascadeProjection2 * position), bias, rotationAngle, 0.25f);
		break;
	case 3:
		shadow = CalculateCascadeShadow(u_ShadowMap3, (u_CascadeProjection3 * position), bias, rotationAngle, 0.125f);
		break;
	}

	return mix(shadow, 1.0f, shadowFade);
}

#endif
