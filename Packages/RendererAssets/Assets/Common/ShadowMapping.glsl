#ifndef SHADOW_MAPPING_H
#define SHADOW_MAPPING_H

#include "Light.glsl"
#include "Math.glsl"

const int CASCADES_COUNT = 4;

layout(std140, set = 0, binding = 2) uniform ShadowData
{
	float u_LightFrustumSize;
	float u_LightSize;
	float u_LightFar;

	vec4 u_SceneScale;
	vec4 u_CascadeSplits;
	vec4 u_CascadeFilterWeights;

	mat4 u_CascadeProjection0;
	mat4 u_CascadeProjection1;
	mat4 u_CascadeProjection2;
	mat4 u_CascadeProjection3;

	float u_ShadowResolution;
	float u_ShadowSoftness;
	float u_ShadowFadeDistance;

	float u_MaxShadowDistance;

	float u_Bias;
	float u_NormalBias;

	int u_MaxCascadeIndex;

};

layout(set = 0, binding = 28) uniform sampler2D u_ShadowMap0;
layout(set = 0, binding = 29) uniform sampler2D u_ShadowMap1;
layout(set = 0, binding = 30) uniform sampler2D u_ShadowMap2;
layout(set = 0, binding = 31) uniform sampler2D u_ShadowMap3;

layout(set = 0, binding = 32) uniform sampler2DShadow u_ShadowMapCompareSampler0;
layout(set = 0, binding = 33) uniform sampler2DShadow u_ShadowMapCompareSampler1;
layout(set = 0, binding = 34) uniform sampler2DShadow u_ShadowMapCompareSampler2;
layout(set = 0, binding = 35) uniform sampler2DShadow u_ShadowMapCompareSampler3;

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

float CalculateAdaptiveEpsilon(float depth, vec3 surfaceNormal, float sceneScale)
{
	float lightFar = 1000.0f;
	float a = (lightFar - depth * (lightFar - u_LightNear));
	float b = lightFar * u_LightNear * (lightFar - u_LightNear);

	float NdotL = dot(-u_LightDirection, surfaceNormal);
	float scaleFactor = min(100.0f, 1 / max(0.0001f, NdotL * NdotL));

	float K = 0.0001f;

	return a * a / b * sceneScale * K * scaleFactor;
}

vec2 GetTexelCenter(vec2 uv)
{
	vec2 pixelCoordinate = uv * u_ShadowResolution;
	return floor(pixelCoordinate) + vec2(0.5f) / u_ShadowResolution;
}

float RayPlaneIntersection(vec4 planeParams, vec3 rayOrigin, vec3 rayDirection)
{
	return -(planeParams.w + dot(planeParams.xyz, rayOrigin)) / dot(rayDirection, planeParams.xyz);
}

// uv in range [0; 1]
void UVToRay(vec2 uv, mat4 inverseProjection, out vec3 origin, out vec3 direction)
{
	uv = uv * 2.0f - vec2(1.0f);

	vec4 rayOrigin = inverseProjection * vec4(uv, 0.0f, 1.0f);

	origin = rayOrigin.xyz / rayOrigin.w;
	direction = u_LightDirection;
}

float FindPotentialOccluder(vec2 uv, mat4 projection, vec3 surfacePosition, vec3 surfaceNormal)
{
	// 1. Create a ray going from the light source through the center of the current shadow map texel
	vec2 texelCenter = GetTexelCenter(uv);
	
	vec3 rayOrigin;
	vec3 rayDirection;
	mat4 inverseProjection = inverse(projection);
	UVToRay(uv, inverseProjection, rayOrigin, rayDirection);

	// 2. Create plane tagent to the surface (in view space)
	vec4 planeParams = vec4(surfaceNormal, -dot(surfaceNormal, surfacePosition));

	// 3. Find ray & plane intersection
	float intersectionDistance = RayPlaneIntersection(planeParams, rayOrigin, rayDirection);
	vec3 intersectionPoint = rayOrigin + rayDirection * intersectionDistance;

	// 4. Projecte intersection plane
	vec4 projectedIntersection = projection * vec4(intersectionPoint, 1.0f);
	projectedIntersection /= projectedIntersection.w;

	return projectedIntersection.z;
}

vec3 CalculateBiasParams(vec2 uv, mat4 projection, vec3 surfacePosition, vec3 surfaceNormal)
{
	float depth = FindPotentialOccluder(uv, projection, surfacePosition, surfaceNormal);
	float depthX = FindPotentialOccluder(uv + vec2(1.0f / u_ShadowResolution, 0.0f), projection, surfacePosition, surfaceNormal);
	float depthY = FindPotentialOccluder(uv + vec2(0.0f, 1.0f / u_ShadowResolution), projection, surfacePosition, surfaceNormal);

	return vec3(depthX - depth, depthY - depth, depth);
}

struct ShadowMappingSurfaceParams
{
	vec3 Position;
	vec3 Normal;
	vec3 BiasParams;
	float ConstantBias;
	vec2 SamplesRotation;
};

float CalculateBlockerDistance(sampler2D shadowMap, vec2 uv, in ShadowMappingSurfaceParams params, float scale)
{
	float recieverDepth = params.BiasParams.z;
	float blockerDistance = 0.0;
	float samplesCount = 0;
	float searchSize = LIGHT_SIZE * (recieverDepth - u_LightNear) / recieverDepth * scale;

	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		vec2 offset = vec2(
			params.SamplesRotation.x * SAMPLE_POINTS[i].x - params.SamplesRotation.y * SAMPLE_POINTS[i].y,
			params.SamplesRotation.y * SAMPLE_POINTS[i].x + params.SamplesRotation.x * SAMPLE_POINTS[i].y
		) * searchSize;

		float newRecieverDepth = params.BiasParams.z + dot(offset, params.BiasParams.xy);
		float sampledDepth = texture(shadowMap, uv + offset).r;
		// float epsilon = CalculateAdaptiveEpsilon(newRecieverDepth, params.Normal, params.SceneScale);

		if (newRecieverDepth > sampledDepth)
		{
			samplesCount += 1.0;
			blockerDistance += sampledDepth;
		}
	}

	if (samplesCount == 0.0)
		return -1.0;
	
	return max(0.0001, blockerDistance / samplesCount);
}

float PCF(sampler2DShadow compareSampler, vec2 uv, float filterRadius, in ShadowMappingSurfaceParams params)
{
	float shadow = 0.0f;
	for (int i = 0; i < NUMBER_OF_SAMPLES; i++)
	{
		vec2 offset = filterRadius * vec2(
			params.SamplesRotation.x * SAMPLE_POINTS[i].x - params.SamplesRotation.y * SAMPLE_POINTS[i].y,
			params.SamplesRotation.y * SAMPLE_POINTS[i].x + params.SamplesRotation.x * SAMPLE_POINTS[i].y
		);

		float recieverDepth = params.BiasParams.z + dot(offset, params.BiasParams.xy);
		// float epsilon = CalculateAdaptiveEpsilon(newRecieverDepth, params.Normal, params.SceneScale);

#if 0
		float sampledDepth = texture(shadowMap, uv + offset).r;
		shadow += (recieverDepth > sampledDepth ? 1.0 : 0.0);
#else
		shadow += 1.0f - texture(compareSampler, vec3(uv + offset, recieverDepth));
#endif
	}
	
	return shadow / NUMBER_OF_SAMPLES;
}

float CalculateCascadeShadow(sampler2D shadowMap, sampler2DShadow compareSampler, mat4 projection, inout ShadowMappingSurfaceParams params, float scale)
{
	vec4 lightSpacePosition = projection * vec4(params.Position, 1.0f);

	vec3 projected = lightSpacePosition.xyz / lightSpacePosition.w;
	projected.xy = projected.xy * 0.5 + vec2(0.5);

	vec2 uv = projected.xy;
	float recieverDepth = projected.z;

	if (recieverDepth > 1.0)
		return 1.0;

	if (projected.x > 1.0 || projected.y > 1.0 || projected.x < 0 || projected.y < 0)
		return 1.0;

	params.BiasParams = CalculateBiasParams(uv, projection, params.Position.xyz, params.Normal);
	params.BiasParams.z -= params.ConstantBias;

	float blockerDistance = CalculateBlockerDistance(shadowMap, uv, params, scale);
	if (blockerDistance == -1.0f)
	{
		return texture(compareSampler, vec3(uv, params.BiasParams.z));
	}

	float penumbraWidth = (recieverDepth - blockerDistance) / blockerDistance;
	float filterRadius = penumbraWidth * LIGHT_SIZE * u_LightNear / recieverDepth;
	filterRadius = filterRadius * scale * u_ShadowSoftness;

	return 1.0f - PCF(compareSampler, uv, filterRadius, params);
}

float CalculateShadow(int cascadeIndex, inout ShadowMappingSurfaceParams params)
{
	switch (cascadeIndex)
	{
	case 0:
		return CalculateCascadeShadow(u_ShadowMap0, u_ShadowMapCompareSampler0, u_CascadeProjection0, params, 1.0f);
	case 1:
		return CalculateCascadeShadow(u_ShadowMap1, u_ShadowMapCompareSampler1, u_CascadeProjection1, params, u_CascadeFilterWeights.y);
	case 2:
		return CalculateCascadeShadow(u_ShadowMap2, u_ShadowMapCompareSampler2, u_CascadeProjection2, params, u_CascadeFilterWeights.z);
	case 3:
		return CalculateCascadeShadow(u_ShadowMap3, u_ShadowMapCompareSampler3, u_CascadeProjection3, params, u_CascadeFilterWeights.w);
	}

	return 1.0f;
}

int CalculateCascadeIndex(vec3 viewSpacePosition)
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

	return cascadeIndex;
}

#ifndef CASCADE_BLENDING_ENABLED
#define CASCADE_BLENDING_ENABLED 1
#endif

#ifndef DEBUG_CASCADES
#define DEBUG_CASCADES 0
#endif

float CalculateShadow(vec3 N, vec4 position, vec3 viewSpacePosition)
{
	float NoL = dot(N, -u_LightDirection);
	float bias = max(u_NormalBias * (1.0f - NoL), 0.0f) + u_Bias;

	if (NoL <= 0.0f)
		return 0.0f;

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
	float rotationAngle = 2.0f * PI * InterleavedGradientNoise(gl_FragCoord.xy);

	ShadowMappingSurfaceParams params;
	params.Position = position.xyz;
	params.Normal = N;
	params.ConstantBias = bias;
	params.SamplesRotation = vec2(cos(rotationAngle), sin(rotationAngle));

	float shadow = CalculateShadow(cascadeIndex, params);

#if CASCADE_BLENDING_ENABLED
	const float BLENDING_THRESHOLD = 1.0f;

	float distanceToNextCascade = u_CascadeSplits[cascadeIndex] - viewSpaceDistance;
	if (distanceToNextCascade <= BLENDING_THRESHOLD && cascadeIndex < CASCADES_COUNT - 1)
	{
		int nextCascade = cascadeIndex + 1;
		float shadow1 = CalculateShadow(nextCascade, params);
		float cascadeBlend = min(1.0f, distanceToNextCascade / BLENDING_THRESHOLD);

		shadow = mix(shadow1, shadow, cascadeBlend);
	}
#endif

	return mix(shadow, 1.0f, shadowFade);
}

#endif
