#ifndef SPOT_LIGHT_SHADOWS_H
#define SPOT_LIGHT_SHADOWS_H

#include "ShadowMapping.glsl"
#include "Light.glsl"

layout(set = 1, binding = 13) uniform sampler2DShadow u_SpotLightShadowMap;

struct SpotLightShadowsEntry
{
	mat4 Projection;
	float Bias;
	float NormalBias;
	uint UVTransform; // [4 bits - log2 size] [14 bits - x offset] [14 bits - y offset]
	float FilterRadius;
};

const uint TILE_SIZE_OFFSET = 28;
const uint TILE_POSITION_MASK = 0x3fff;
const uint TILE_POSITION_OFFSET = 14;

layout(std430, set = 1, binding = 14) readonly buffer SpotLightShadowData
{
	SpotLightShadowsEntry Entries[];
} u_SpotLightShadowData;

float CalculateSpotLightShadow(vec3 spotLightPosition,
	vec3 surfaceNormal,
	vec3 position,
	uint lightIndex,
	vec3 directionFromLight)
{
	float NoL = dot(-directionFromLight, surfaceNormal);
	if (NoL <= 0.0f)
		return 0.0f;

	SpotLightShadowsEntry shadowEntry = u_SpotLightShadowData.Entries[lightIndex];
	vec4 projected = shadowEntry.Projection * vec4(position, 1.0f);
	projected /= projected.w;

	vec2 uv = projected.xy * 0.5f + vec2(0.5f);
	if (any(lessThan(uv, vec2(0.0f))) || any(greaterThan(uv, vec2(1.0f))) || projected.z >= 1.0f)
		return 1.0f;

	float size = float(1 << (shadowEntry.UVTransform >> TILE_SIZE_OFFSET));
	vec2 texelCenter = floor(uv * size + vec2(0.5)) / size;

	float bias = max(shadowEntry.NormalBias * (1.0f - NoL), 0.0f) + shadowEntry.Bias;
	
	float rotationAngle = 2.0f * PI * InterleavedGradientNoise(gl_FragCoord.xy);
	ShadowMappingSurfaceParams params;
	params.Position = position;
	params.Normal = surfaceNormal;
	params.ConstantBias = 0.0f;
	params.BiasParams = vec3(0.0f, 0.0f, projected.z - bias);
	params.SamplesRotation = vec2(cos(rotationAngle), sin(rotationAngle));

	uvec2 offset = uvec2(
		shadowEntry.UVTransform >> TILE_POSITION_OFFSET,
		shadowEntry.UVTransform) & TILE_POSITION_MASK;

	uv.xy = (size * uv.xy + vec2(offset)) * u_SpotLightsShadowAtlasTexelSize;
	return 1.0f - PCF(u_SpotLightShadowMap, uv, shadowEntry.FilterRadius * u_SpotLightsShadowAtlasTexelSize, params);
}

vec3 ComputeShadowCastingSpotLightsContribution(vec3 V, in SurfaceProperties surface)
{
	vec3 finalContribution = vec3(0.0f);

	for (uint i = 0; i < u_ShadowCastingSpotlightCount; i++)
	{
		uint spotlightIndex = i + u_FirstShadowCastingSpotlight;
		SpotLightData spotLight = u_SpotLights[spotlightIndex];

		vec3 directionFromLight = surface.Position - spotLight.Position;

		vec3 light = CalculateSingleSpotLightContricbution(V, surface, spotlightIndex);
		float shadow = CalculateSpotLightShadow(spotLight.Position,
			surface.Normal,
			surface.Position,
			i, normalize(directionFromLight));

		finalContribution += light * shadow;
	}

	return finalContribution;
}

#endif
